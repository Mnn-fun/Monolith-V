using System.Text.Json;
using Microsoft.Extensions.Logging.Abstractions;
using MonolithV.Data;
using StackExchange.Redis;
using Xunit;

namespace MonolithV.Tests;

public class CachedPlayerRepositoryTests : IAsyncLifetime
{
    private class FakeInnerRepository : IPlayerRepository
    {
        public int CallCount { get; private set; }
        private readonly PlayerDto? _playerToReturn;

        public FakeInnerRepository(PlayerDto? playerToReturn)
        {
            _playerToReturn = playerToReturn;
        }

        public Task<PlayerDto?> GetByEosAccountIdAsync(string eosAccountId, CancellationToken cancellationToken = default)
        {
            CallCount++;
            return Task.FromResult(_playerToReturn);
        }
    }

    private class ThrowingInnerRepository : IPlayerRepository
    {
        public Task<PlayerDto?> GetByEosAccountIdAsync(string eosAccountId, CancellationToken cancellationToken = default)
        {
            throw new InvalidOperationException("Oracle database error");
        }
    }

    private IConnectionMultiplexer? _redis;
    private bool _redisAvailable;

    public async Task InitializeAsync()
    {
        try
        {
            var options = ConfigurationOptions.Parse("localhost:6379");
            options.ConnectTimeout = 1000;
            options.AbortOnConnectFail = true;
            _redis = await ConnectionMultiplexer.ConnectAsync(options);
            _redisAvailable = _redis.IsConnected;
        }
        catch
        {
            _redisAvailable = false;
        }
    }

    public async Task DisposeAsync()
    {
        if (_redis is not null)
        {
            await _redis.CloseAsync();
            _redis.Dispose();
        }
    }

    [Fact]
    public async Task GetByEosAccountIdAsync_OnCacheMiss_CallsInnerRepositoryAndCachesInRedis()
    {
        if (!_redisAvailable || _redis is null)
        {
            // Skip test if Redis container isn't reachable during test run
            return;
        }

        var db = _redis.GetDatabase();
        var testEosId = $"test-miss-{Guid.NewGuid()}";
        var cacheKey = $"player:{testEosId}";
        await db.KeyDeleteAsync(cacheKey);

        var expectedPlayer = new PlayerDto("pid-1", testEosId, "Hero", DateTimeOffset.UtcNow);
        var innerRepo = new FakeInnerRepository(expectedPlayer);
        var cachedRepo = new CachedPlayerRepository(innerRepo, _redis, NullLogger<CachedPlayerRepository>.Instance);

        // Act: First call (cache miss)
        var result1 = await cachedRepo.GetByEosAccountIdAsync(testEosId);

        // Assert: Inner called once, result returned and cached
        Assert.Equal(1, innerRepo.CallCount);
        Assert.NotNull(result1);
        Assert.Equal(testEosId, result1!.EosAccountId);

        var cachedJson = await db.StringGetAsync(cacheKey);
        Assert.True(cachedJson.HasValue);

        // Act: Second call (cache hit)
        var result2 = await cachedRepo.GetByEosAccountIdAsync(testEosId);

        // Assert: Inner call count still 1 (served from Redis)
        Assert.Equal(1, innerRepo.CallCount);
        Assert.NotNull(result2);
        Assert.Equal(testEosId, result2!.EosAccountId);

        // Cleanup
        await db.KeyDeleteAsync(cacheKey);
    }

    [Fact]
    public async Task GetByEosAccountIdAsync_WhenRedisIsDownOrThrows_FallsBackToInnerRepositoryWithoutCrashing()
    {
        // Arrange: Connect to a non-existent port or use broken multiplexer options if possible,
        // or simulate Redis failure gracefully.
        var innerPlayer = new PlayerDto("pid-fb", "eos-fb", "FallbackHero", DateTimeOffset.UtcNow);
        var innerRepo = new FakeInnerRepository(innerPlayer);

        // Create options pointing to an invalid address so Redis throws on operations
        var brokenOptions = new ConfigurationOptions { AbortOnConnectFail = false };
        brokenOptions.EndPoints.Add("localhost:1"); // closed port
        brokenOptions.ConnectTimeout = 100;
        using var brokenRedis = await ConnectionMultiplexer.ConnectAsync(brokenOptions);

        var cachedRepo = new CachedPlayerRepository(innerRepo, brokenRedis, NullLogger<CachedPlayerRepository>.Instance);

        // Act: Call repository when Redis connection is dead
        var result = await cachedRepo.GetByEosAccountIdAsync("eos-fb");

        // Assert: Did not throw, called inner Oracle repository
        Assert.Equal(1, innerRepo.CallCount);
        Assert.NotNull(result);
        Assert.Equal("FallbackHero", result!.DisplayName);
    }
}
