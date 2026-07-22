using System.Collections.Concurrent;
using System.Data;
using Oracle.ManagedDataAccess.Client;

namespace MonolithV.Data;

public record CheckpointClaimResult(
    bool Success,
    bool AlreadyClaimed,
    string? Message = null
);

public interface ICheckpointRepository
{
    Task<CheckpointClaimResult> RecordCheckpointProgressAsync(
        string seasonId,
        string playerId,
        int checkpointIndex,
        CancellationToken cancellationToken = default);

    Task<int> GetLatestCheckpointIndexAsync(
        string seasonId,
        string playerId,
        CancellationToken cancellationToken = default);
}

/// <summary>
/// Concurrency-safe repository for player checkpoint claim and retrieval.
/// Implements keyed in-process semaphore locking ($"{seasonId}:{playerId}:{checkpointIndex}") and
/// Oracle ORA-00001 primary key constraint catching to ensure atomic, idempotent progression updates.
/// </summary>
public class CheckpointRepository : ICheckpointRepository
{
    private readonly IOracleConnectionFactory _connectionFactory;
    private static readonly ConcurrentDictionary<string, SemaphoreSlim> _semaphores = new();

    public CheckpointRepository(IOracleConnectionFactory connectionFactory)
    {
        _connectionFactory = connectionFactory;
    }

    public async Task<CheckpointClaimResult> RecordCheckpointProgressAsync(
        string seasonId,
        string playerId,
        int checkpointIndex,
        CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(seasonId);
        ArgumentException.ThrowIfNullOrWhiteSpace(playerId);

        if (checkpointIndex < 0)
        {
            return new CheckpointClaimResult(false, false, "Checkpoint index must be non-negative.");
        }

        var lockKey = $"{seasonId}:{playerId}:{checkpointIndex}";
        var semaphore = _semaphores.GetOrAdd(lockKey, _ => new SemaphoreSlim(1, 1));

        await semaphore.WaitAsync(cancellationToken).ConfigureAwait(false);
        try
        {
            await using var connection = await _connectionFactory.CreateOpenConnectionAsync(cancellationToken).ConfigureAwait(false);
            await using var transaction = (OracleTransaction)await connection.BeginTransactionAsync(cancellationToken).ConfigureAwait(false);

            try
            {
                await using var insertCommand = connection.CreateCommand();
                insertCommand.Transaction = transaction;
                insertCommand.CommandText = @"
                    INSERT INTO checkpoint_progress (player_id, season_id, checkpoint_index, reached_at)
                    VALUES (:playerId, :seasonId, :checkpointIndex, CURRENT_TIMESTAMP)";

                insertCommand.Parameters.Add(new OracleParameter("playerId", OracleDbType.Varchar2, playerId, ParameterDirection.Input));
                insertCommand.Parameters.Add(new OracleParameter("seasonId", OracleDbType.Varchar2, seasonId, ParameterDirection.Input));
                insertCommand.Parameters.Add(new OracleParameter("checkpointIndex", OracleDbType.Int32, checkpointIndex, ParameterDirection.Input));

                await insertCommand.ExecuteNonQueryAsync(cancellationToken).ConfigureAwait(false);
                await transaction.CommitAsync(cancellationToken).ConfigureAwait(false);

                return new CheckpointClaimResult(true, false, "Checkpoint successfully claimed.");
            }
            catch (Exception ex) when (IsUniqueConstraintViolation(ex))
            {
                await transaction.RollbackAsync(cancellationToken).ConfigureAwait(false);
                return new CheckpointClaimResult(true, true, "Checkpoint already claimed.");
            }
        }
        finally
        {
            semaphore.Release();
        }
    }

    public async Task<int> GetLatestCheckpointIndexAsync(
        string seasonId,
        string playerId,
        CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(seasonId);
        ArgumentException.ThrowIfNullOrWhiteSpace(playerId);

        await using var connection = await _connectionFactory.CreateOpenConnectionAsync(cancellationToken).ConfigureAwait(false);
        await using var command = connection.CreateCommand();
        command.CommandText = @"
            SELECT NVL(MAX(checkpoint_index), -1)
            FROM checkpoint_progress
            WHERE player_id = :playerId AND season_id = :seasonId";

        command.Parameters.Add(new OracleParameter("playerId", OracleDbType.Varchar2, playerId, ParameterDirection.Input));
        command.Parameters.Add(new OracleParameter("seasonId", OracleDbType.Varchar2, seasonId, ParameterDirection.Input));

        var result = await command.ExecuteScalarAsync(cancellationToken).ConfigureAwait(false);
        if (result is null || result == DBNull.Value)
        {
            return -1;
        }

        return Convert.ToInt32(result);
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
                current.Message.Contains("pk_checkpoint_progress", StringComparison.OrdinalIgnoreCase))
            {
                return true;
            }

            current = current.InnerException;
        }
        return false;
    }
}
