using System.Collections.Concurrent;
using System.Data;
using System.Data.Common;
using Oracle.ManagedDataAccess.Client;

namespace MonolithV.Data;

public record ShareEventResult(
    bool Success,
    bool AlreadyShared,
    string? Message = null,
    string? ShareEventId = null
);

public interface IShareEventRepository
{
    Task<ShareEventResult> RecordShareEventAsync(
        string seasonId,
        string giverPlayerId,
        string receiverPlayerId,
        string itemType,
        CancellationToken cancellationToken = default);

    Task<bool> HasPlayerSharedAsync(
        string seasonId,
        string playerId,
        CancellationToken cancellationToken = default);
}

/// <summary>
/// Concurrency-safe repository for recording role-item share events (Golden Apple / Counterpart Item).
/// Implements keyed in-process semaphore locking and database-level unique constraint handling (ORA-00001)
/// to ensure race-free, idempotent transaction execution across concurrent requests.
/// </summary>
public class ShareEventRepository : IShareEventRepository
{
    private readonly IOracleConnectionFactory _connectionFactory;
    private static readonly ConcurrentDictionary<string, SemaphoreSlim> _semaphores = new();

    public ShareEventRepository(IOracleConnectionFactory connectionFactory)
    {
        _connectionFactory = connectionFactory;
    }

    public async Task<ShareEventResult> RecordShareEventAsync(
        string seasonId,
        string giverPlayerId,
        string receiverPlayerId,
        string itemType,
        CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(seasonId);
        ArgumentException.ThrowIfNullOrWhiteSpace(giverPlayerId);
        ArgumentException.ThrowIfNullOrWhiteSpace(receiverPlayerId);
        ArgumentException.ThrowIfNullOrWhiteSpace(itemType);

        if (string.Equals(giverPlayerId, receiverPlayerId, StringComparison.OrdinalIgnoreCase))
        {
            return new ShareEventResult(false, false, "Giver and receiver must be different players.");
        }

        // Keyed async lock: serialize concurrent requests for the exact same pair & item type in memory.
        var lockKey = $"{seasonId}:{giverPlayerId}:{receiverPlayerId}:{itemType}";
        var semaphore = _semaphores.GetOrAdd(lockKey, _ => new SemaphoreSlim(1, 1));

        await semaphore.WaitAsync(cancellationToken).ConfigureAwait(false);
        try
        {
            await using var connection = await _connectionFactory.CreateOpenConnectionAsync(cancellationToken).ConfigureAwait(false);

            // Step 1: Validate giver/receiver season roles inside the semaphore-guarded section.
            await using (var roleCommand = connection.CreateCommand())
            {
                roleCommand.CommandText = @"
                    SELECT player_id, role
                    FROM player_season_roles
                    WHERE season_id = :seasonId AND player_id IN (:giverId, :receiverId)";

                roleCommand.Parameters.Add(new OracleParameter("seasonId", OracleDbType.Varchar2, seasonId, ParameterDirection.Input));
                roleCommand.Parameters.Add(new OracleParameter("giverId", OracleDbType.Varchar2, giverPlayerId, ParameterDirection.Input));
                roleCommand.Parameters.Add(new OracleParameter("receiverId", OracleDbType.Varchar2, receiverPlayerId, ParameterDirection.Input));

                string? giverRole = null;
                string? receiverRole = null;

                await using var reader = await roleCommand.ExecuteReaderAsync(cancellationToken).ConfigureAwait(false);
                while (await reader.ReadAsync(cancellationToken).ConfigureAwait(false))
                {
                    var playerId = reader.GetString(reader.GetOrdinal("player_id"));
                    var role = reader.GetString(reader.GetOrdinal("role"));

                    if (string.Equals(playerId, giverPlayerId, StringComparison.OrdinalIgnoreCase))
                    {
                        giverRole = role;
                    }
                    else if (string.Equals(playerId, receiverPlayerId, StringComparison.OrdinalIgnoreCase))
                    {
                        receiverRole = role;
                    }
                }

                if (string.IsNullOrWhiteSpace(giverRole) || string.IsNullOrWhiteSpace(receiverRole))
                {
                    return new ShareEventResult(false, false, "Both giver and receiver must have chosen a season role in this season.");
                }

                if (string.Equals(giverRole, receiverRole, StringComparison.OrdinalIgnoreCase))
                {
                    return new ShareEventResult(false, false, "Giver and receiver must have opposite season roles ('MALE' vs 'FEMALE').");
                }
            }

            // Step 2: Attempt INSERT inside an explicit Oracle transaction.
            await using var transaction = (OracleTransaction)await connection.BeginTransactionAsync(cancellationToken).ConfigureAwait(false);
            try
            {
                var shareEventId = Guid.NewGuid().ToString("N");

                await using var insertCommand = connection.CreateCommand();
                insertCommand.Transaction = transaction;
                insertCommand.CommandText = @"
                    INSERT INTO share_events (share_event_id, season_id, giver_player_id, receiver_player_id, item_type, shared_at)
                    VALUES (:shareEventId, :seasonId, :giverId, :receiverId, :itemType, CURRENT_TIMESTAMP)";

                insertCommand.Parameters.Add(new OracleParameter("shareEventId", OracleDbType.Varchar2, shareEventId, ParameterDirection.Input));
                insertCommand.Parameters.Add(new OracleParameter("seasonId", OracleDbType.Varchar2, seasonId, ParameterDirection.Input));
                insertCommand.Parameters.Add(new OracleParameter("giverId", OracleDbType.Varchar2, giverPlayerId, ParameterDirection.Input));
                insertCommand.Parameters.Add(new OracleParameter("receiverId", OracleDbType.Varchar2, receiverPlayerId, ParameterDirection.Input));
                insertCommand.Parameters.Add(new OracleParameter("itemType", OracleDbType.Varchar2, itemType, ParameterDirection.Input));

                await insertCommand.ExecuteNonQueryAsync(cancellationToken).ConfigureAwait(false);
                await transaction.CommitAsync(cancellationToken).ConfigureAwait(false);

                return new ShareEventResult(true, false, "Share event successfully recorded.", shareEventId);
            }
            catch (Exception ex) when (IsUniqueConstraintViolation(ex))
            {
                await transaction.RollbackAsync(cancellationToken).ConfigureAwait(false);
                return new ShareEventResult(true, true, "Share event already recorded.", null);
            }
        }
        finally
        {
            semaphore.Release();
        }
    }

    private static bool IsUniqueConstraintViolation(Exception ex)
    {
        var current = ex;
        while (current is not null)
        {
            if (current is OracleException oracleEx && oracleEx.Number == 1)
            {
                return true;
            }

            if (current.Message.Contains("ORA-00001", StringComparison.OrdinalIgnoreCase) ||
                current.Message.Contains("unique constraint", StringComparison.OrdinalIgnoreCase) ||
                current.Message.Contains("uq_share_events", StringComparison.OrdinalIgnoreCase))
            {
                return true;
            }

            current = current.InnerException;
        }
        return false;
    }

    public async Task<bool> HasPlayerSharedAsync(
        string seasonId,
        string playerId,
        CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(seasonId);
        ArgumentException.ThrowIfNullOrWhiteSpace(playerId);

        await using var connection = await _connectionFactory.CreateOpenConnectionAsync(cancellationToken).ConfigureAwait(false);

        await using var command = connection.CreateCommand();
        command.CommandText = @"
            SELECT 1 
            FROM share_events 
            WHERE season_id = :seasonId 
              AND (giver_player_id = :playerId OR receiver_player_id = :playerId)
            FETCH FIRST 1 ROWS ONLY";

        command.Parameters.Add(new OracleParameter("seasonId", OracleDbType.Varchar2, seasonId, ParameterDirection.Input));
        command.Parameters.Add(new OracleParameter("playerId", OracleDbType.Varchar2, playerId, ParameterDirection.Input));

        var result = await command.ExecuteScalarAsync(cancellationToken).ConfigureAwait(false);
        return result != null;
    }
}
