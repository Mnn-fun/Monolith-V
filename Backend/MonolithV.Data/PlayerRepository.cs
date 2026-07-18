using System.Data;
using Oracle.ManagedDataAccess.Client;

namespace MonolithV.Data;

public interface IPlayerRepository
{
    Task<PlayerDto?> GetByEosAccountIdAsync(string eosAccountId, CancellationToken cancellationToken = default);
}

public class PlayerRepository : IPlayerRepository
{
    private readonly IOracleConnectionFactory _connectionFactory;

    public PlayerRepository(IOracleConnectionFactory connectionFactory)
    {
        _connectionFactory = connectionFactory;
    }

    public async Task<PlayerDto?> GetByEosAccountIdAsync(string eosAccountId, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(eosAccountId);

        await using var connection = await _connectionFactory.CreateOpenConnectionAsync(cancellationToken).ConfigureAwait(false);
        await using var command = connection.CreateCommand();

        // Strict bind-variable parameterized query. Never string-concatenate SQL.
        command.CommandText = @"
            SELECT player_id, eos_account_id, display_name, created_at
            FROM players
            WHERE eos_account_id = :eosAccountId";

        command.Parameters.Add(new OracleParameter("eosAccountId", OracleDbType.Varchar2, eosAccountId, ParameterDirection.Input));

        await using var reader = await command.ExecuteReaderAsync(CommandBehavior.SingleRow, cancellationToken).ConfigureAwait(false);

        if (await reader.ReadAsync(cancellationToken).ConfigureAwait(false))
        {
            var playerId = reader.GetString(reader.GetOrdinal("player_id"));
            var eosId = reader.GetString(reader.GetOrdinal("eos_account_id"));
            var displayName = reader.GetString(reader.GetOrdinal("display_name"));

            var createdAtOrdinal = reader.GetOrdinal("created_at");
            var value = reader.GetValue(createdAtOrdinal);
            DateTimeOffset createdAt = value switch
            {
                DateTimeOffset dto => dto,
                DateTime dt => new DateTimeOffset(dt),
                Oracle.ManagedDataAccess.Types.OracleTimeStampTZ tz => tz.Value,
                _ => DateTimeOffset.Parse(value.ToString()!)
            };

            return new PlayerDto(playerId, eosId, displayName, createdAt);
        }

        return null;
    }
}
