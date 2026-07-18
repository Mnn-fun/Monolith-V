using Microsoft.Extensions.Configuration;
using Oracle.ManagedDataAccess.Client;

namespace MonolithV.Data;

/// <summary>
/// Factory for creating and opening async Oracle database connections.
/// Enforces non-blocking connection initialization.
/// </summary>
public interface IOracleConnectionFactory
{
    Task<OracleConnection> CreateOpenConnectionAsync(CancellationToken cancellationToken = default);
}

public class OracleConnectionFactory : IOracleConnectionFactory
{
    private readonly IConfiguration _configuration;

    public OracleConnectionFactory(IConfiguration configuration)
    {
        _configuration = configuration;
    }

    public async Task<OracleConnection> CreateOpenConnectionAsync(CancellationToken cancellationToken = default)
    {
        var connectionString = _configuration.GetConnectionString("Oracle")
            ?? throw new InvalidOperationException("Connection string 'Oracle' not found in configuration.");

        var connection = new OracleConnection(connectionString);
        await connection.OpenAsync(cancellationToken).ConfigureAwait(false);
        return connection;
    }
}
