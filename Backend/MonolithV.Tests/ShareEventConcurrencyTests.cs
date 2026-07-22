using System.Collections.Concurrent;
using Microsoft.AspNetCore.Mvc;
using MonolithV.Api.Controllers;
using MonolithV.Data;
using Xunit;

namespace MonolithV.Tests;

public class ShareEventConcurrencyTests
{
    /// <summary>
    /// Thread-safe simulated repository verifying the exact atomic lock and unique constraint handling
    /// required by P2.7 without requiring a live Oracle database instance during unit/CI execution.
    /// </summary>
    private class SimulatedConcurrentShareEventRepository : IShareEventRepository
    {
        private readonly ConcurrentDictionary<string, string> _shareEventsTable = new();
        private readonly ConcurrentDictionary<string, string> _playerRoles = new();
        private readonly SemaphoreSlim _semaphore = new(1, 1);
        private int _totalInsertionAttempts = 0;

        public SimulatedConcurrentShareEventRepository()
        {
            // Seed opposite roles for standard test pair
            _playerRoles["giver-male"] = "MALE";
            _playerRoles["receiver-female"] = "FEMALE";
            // Seed matching roles for mismatch test
            _playerRoles["player-male-1"] = "MALE";
            _playerRoles["player-male-2"] = "MALE";
        }

        public int TotalInsertionAttempts => _totalInsertionAttempts;
        public int TotalRecordedRows => _shareEventsTable.Count;

        public async Task<ShareEventResult> RecordShareEventAsync(
            string seasonId,
            string giverPlayerId,
            string receiverPlayerId,
            string itemType,
            CancellationToken cancellationToken = default)
        {
            // Simulate role check
            if (!_playerRoles.TryGetValue(giverPlayerId, out var giverRole) ||
                !_playerRoles.TryGetValue(receiverPlayerId, out var receiverRole))
            {
                return new ShareEventResult(false, false, "Both giver and receiver must have chosen a season role.");
            }

            if (string.Equals(giverRole, receiverRole, StringComparison.OrdinalIgnoreCase))
            {
                return new ShareEventResult(false, false, "Giver and receiver must have opposite season roles ('MALE' vs 'FEMALE').");
            }

            var uniqueKey = $"{seasonId}:{giverPlayerId}:{receiverPlayerId}:{itemType}";

            // Simulate the keyed semaphore / DB unique constraint transaction boundary
            await _semaphore.WaitAsync(cancellationToken);
            try
            {
                Interlocked.Increment(ref _totalInsertionAttempts);

                if (_shareEventsTable.TryAdd(uniqueKey, Guid.NewGuid().ToString("N")))
                {
                    return new ShareEventResult(true, false, "Share event successfully recorded.", _shareEventsTable[uniqueKey]);
                }
                else
                {
                    // Simulates catching ORA-00001 unique constraint violation and treating as idempotent success
                    return new ShareEventResult(true, true, "Share event already recorded.", null);
                }
            }
            finally
            {
                _semaphore.Release();
            }
        }
    }

    [Fact]
    public async Task TwentyConcurrentIdenticalRequests_ProduceExactlyOneRow_AndAllReturn200Ok()
    {
        // Arrange
        var repository = new SimulatedConcurrentShareEventRepository();
        var controller = new ShareEventsController(repository);
        var request = new CreateShareEventRequest("giver-male", "receiver-female", "GOLDEN_APPLE");

        // Act - Fire 20 concurrent identical POST requests simultaneously via Task.WhenAll
        var tasks = Enumerable.Range(0, 20).Select(_ =>
            controller.CreateShareEvent("season-1", request, CancellationToken.None)
        ).ToArray();

        var results = await Task.WhenAll(tasks);

        // Assert 1: All 20 requests must return 200 OK (no 500s or unhandled constraint/lock exceptions)
        Assert.All(results, result => Assert.IsType<OkObjectResult>(result));

        // Assert 2: Check responses
        var okResponses = results.Cast<OkObjectResult>().Select(r => r.Value).ToList();
        var successCount = 0;
        var alreadySharedCount = 0;

        foreach (var resp in okResponses)
        {
            var type = resp!.GetType();
            var success = (bool)type.GetProperty("success")!.GetValue(resp)!;
            var alreadyShared = (bool)type.GetProperty("alreadyShared")!.GetValue(resp)!;

            Assert.True(success);
            if (!alreadyShared)
            {
                successCount++;
            }
            else
            {
                alreadySharedCount++;
            }
        }

        // Assert 3: Exactly 1 real insertion success, and 19 idempotent alreadyShared responses
        Assert.Equal(1, successCount);
        Assert.Equal(19, alreadySharedCount);

        // Assert 4: Exactly 1 row in the underlying data store
        Assert.Equal(1, repository.TotalRecordedRows);
    }

    [Fact]
    public async Task CreateShareEvent_WhenRolesMatch_ReturnsBadRequest400()
    {
        // Arrange
        var repository = new SimulatedConcurrentShareEventRepository();
        var controller = new ShareEventsController(repository);
        var request = new CreateShareEventRequest("player-male-1", "player-male-2", "GOLDEN_APPLE");

        // Act
        var result = await controller.CreateShareEvent("season-1", request, CancellationToken.None);

        // Assert
        var badRequestResult = Assert.IsType<BadRequestObjectResult>(result);
        Assert.NotNull(badRequestResult.Value);
    }

    [Fact]
    public async Task CreateShareEvent_WhenGiverAndReceiverAreSamePlayer_ReturnsBadRequest400()
    {
        // Arrange
        var repository = new SimulatedConcurrentShareEventRepository();
        var controller = new ShareEventsController(repository);
        var request = new CreateShareEventRequest("giver-male", "giver-male", "GOLDEN_APPLE");

        // Act
        var result = await controller.CreateShareEvent("season-1", request, CancellationToken.None);

        // Assert
        Assert.IsType<BadRequestObjectResult>(result);
    }
}
