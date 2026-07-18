using System.Text.Json;
using Microsoft.Extensions.Logging;
using StackExchange.Redis;

namespace MonolithV.Data;

/// <summary>
/// Cache-aside decorator for <see cref="IPlayerRepository"/>.
/// Checks Redis before calling the underlying Oracle database repository.
/// Enforces a strict 60-second TTL so Redis acts purely as a transient cache, never the source of truth.
/// </summary>
public class CachedPlayerRepository : IPlayerRepository
{
    private readonly IPlayerRepository _innerRepository;
    private readonly IConnectionMultiplexer _redis;
    private readonly ILogger<CachedPlayerRepository> _logger;
    private static readonly TimeSpan DefaultTtl = TimeSpan.FromSeconds(60);

    public CachedPlayerRepository(
        IPlayerRepository innerRepository,
        IConnectionMultiplexer redis,
        ILogger<CachedPlayerRepository> logger)
    {
        _innerRepository = innerRepository;
        _redis = redis;
        _logger = logger;
    }

    public async Task<PlayerDto?> GetByEosAccountIdAsync(string eosAccountId, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(eosAccountId);

        var cacheKey = $"player:{eosAccountId}";
        var database = _redis.GetDatabase();

        try
        {
            var cachedJson = await database.StringGetAsync(cacheKey).ConfigureAwait(false);
            if (!cachedJson.IsNullOrEmpty)
            {
                _logger.LogInformation("[Cache Hit] Found profile for {EosAccountId} in Redis cache.", eosAccountId);
                var cachedDto = JsonSerializer.Deserialize<PlayerDto>((string)cachedJson!);
                if (cachedDto is not null)
                {
                    return cachedDto;
                }
            }
        }
        catch (Exception ex)
        {
            // Redis failures must never bring down the API; log and fall back gracefully to Oracle.
            _logger.LogWarning(ex, "[Cache Error] Failed reading {CacheKey} from Redis. Falling back to Oracle.", cacheKey);
        }

        _logger.LogInformation("[Cache Miss] Profile for {EosAccountId} not in Redis. Querying Oracle database...", eosAccountId);
        var playerDto = await _innerRepository.GetByEosAccountIdAsync(eosAccountId, cancellationToken).ConfigureAwait(false);

        if (playerDto is not null)
        {
            try
            {
                var json = JsonSerializer.Serialize(playerDto);
                await database.StringSetAsync(cacheKey, json, DefaultTtl).ConfigureAwait(false);
                _logger.LogInformation("[Cache Store] Saved {EosAccountId} to Redis with {Ttl}s TTL.", eosAccountId, DefaultTtl.TotalSeconds);
            }
            catch (Exception ex)
            {
                _logger.LogWarning(ex, "[Cache Error] Failed writing {CacheKey} to Redis.", cacheKey);
            }
        }

        return playerDto;
    }
}
