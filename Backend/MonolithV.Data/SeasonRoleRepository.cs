using Oracle.ManagedDataAccess.Client;
using System.Data;
using System.Data.Common;
using System.Threading;
using System.Threading.Tasks;

namespace MonolithV.Data;

public class SeasonRoleRepository : ISeasonRoleRepository
{
    private readonly IOracleConnectionFactory _connectionFactory;

    public SeasonRoleRepository(IOracleConnectionFactory connectionFactory)
    {
        _connectionFactory = connectionFactory;
    }

    public async Task<string?> GetRoleAsync(string seasonId, string playerId, CancellationToken cancellationToken)
    {
        using var connection = await _connectionFactory.CreateOpenConnectionAsync(cancellationToken).ConfigureAwait(false);

        using var command = connection.CreateCommand();
        command.CommandText = @"
            SELECT role 
            FROM player_season_roles 
            WHERE season_id = :seasonId AND player_id = :playerId";
        
        command.Parameters.Add(new OracleParameter("seasonId", seasonId));
        command.Parameters.Add(new OracleParameter("playerId", playerId));

        using var reader = await command.ExecuteReaderAsync(cancellationToken).ConfigureAwait(false);
        if (await reader.ReadAsync(cancellationToken).ConfigureAwait(false))
        {
            return reader.GetString(0);
        }
        return null;
    }

    public async Task<bool> AssignRoleAsync(string seasonId, string playerId, string role, CancellationToken cancellationToken)
    {
        using var connection = await _connectionFactory.CreateOpenConnectionAsync(cancellationToken).ConfigureAwait(false);

        using var command = connection.CreateCommand();
        command.CommandText = @"
            INSERT INTO player_season_roles (player_id, season_id, role)
            VALUES (:playerId, :seasonId, :role)";
        
        command.Parameters.Add(new OracleParameter("playerId", playerId));
        command.Parameters.Add(new OracleParameter("seasonId", seasonId));
        command.Parameters.Add(new OracleParameter("role", role));

        try
        {
            await command.ExecuteNonQueryAsync(cancellationToken).ConfigureAwait(false);
            return true;
        }
        catch (OracleException ex) when (IsUniqueConstraintViolation(ex))
        {
            // ORA-00001: unique constraint violated (pk_player_season_roles)
            return false;
        }
    }

    private static bool IsUniqueConstraintViolation(OracleException ex)
    {
        return ex.Number == 1; // ORA-00001
    }
}
