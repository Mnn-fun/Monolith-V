using System.Collections.Concurrent;
using Microsoft.AspNetCore.Mvc;
using MonolithV.Api.Controllers;
using MonolithV.Data;
using Xunit;

namespace MonolithV.Tests;

public class CheckpointConcurrencyTests
{
    /// <summary>
    /// Thread-safe simulated repository verifying the exact atomic lock and primary key constraint handling
    /// required by P2.8 without requiring a live Oracle database instance during unit/CI execution.
    /// </summary>
    private class SimulatedConcurrentCheckpointRepository : ICheckpointRepository
    {
        private readonly ConcurrentDictionary<string, int> _checkpointsTable = new();
        private readonly SemaphoreSlim _semaphore = new(1, 1);

        public int TotalRecordedRows => _checkpointsTable.Count;

        public async Task<CheckpointClaimResult> RecordCheckpointProgressAsync(
            string seasonId,
            string playerId,
            int checkpointIndex,
            CancellationToken cancellationToken = default)
        {
            if (checkpointIndex < 0)
            {
                return new CheckpointClaimResult(false, false, "Checkpoint index must be non-negative.");
            }

            var uniqueKey = $"{seasonId}:{playerId}:{checkpointIndex}";

            await _semaphore.WaitAsync(cancellationToken);
            try
            {
                if (_checkpointsTable.TryAdd(uniqueKey, checkpointIndex))
                {
                    return new CheckpointClaimResult(true, false, "Checkpoint successfully claimed.");
                }
                else
                {
                    // Simulates catching ORA-00001 primary key violation and treating as idempotent success
                    return new CheckpointClaimResult(true, true, "Checkpoint already claimed.");
                }
            }
            finally
            {
                _semaphore.Release();
            }
        }

        public Task<int> GetLatestCheckpointIndexAsync(
            string seasonId,
            string playerId,
            CancellationToken cancellationToken = default)
        {
            var prefix = $"{seasonId}:{playerId}:";
            var matchingIndices = _checkpointsTable
                .Where(kv => kv.Key.StartsWith(prefix))
                .Select(kv => kv.Value)
                .ToList();

            if (matchingIndices.Count == 0)
            {
                return Task.FromResult(-1);
            }

            return Task.FromResult(matchingIndices.Max());
        }
    }

    [Fact]
    public async Task TwentyConcurrentIdenticalCheckpointClaims_ProduceExactlyOneRow_AndAllReturn200Ok()
    {
        // Arrange
        var repository = new SimulatedConcurrentCheckpointRepository();
        var controller = new CheckpointsController(repository);
        var request = new ClaimCheckpointRequest(1);

        // Act - Fire 20 concurrent identical POST requests simultaneously via Task.WhenAll
        var tasks = Enumerable.Range(0, 20).Select(_ =>
            controller.ClaimCheckpoint("season-1", "player-alpha", request, CancellationToken.None)
        ).ToArray();

        var results = await Task.WhenAll(tasks);

        // Assert 1: All 20 requests must return 200 OK (no 500s or unhandled primary key/lock exceptions)
        Assert.All(results, result => Assert.IsType<OkObjectResult>(result));

        // Assert 2: Check responses
        var okResponses = results.Cast<OkObjectResult>().Select(r => r.Value).ToList();
        var successCount = 0;
        var alreadyClaimedCount = 0;

        foreach (var resp in okResponses)
        {
            var type = resp!.GetType();
            var success = (bool)type.GetProperty("success")!.GetValue(resp)!;
            var alreadyClaimed = (bool)type.GetProperty("alreadyClaimed")!.GetValue(resp)!;

            Assert.True(success);
            if (!alreadyClaimed)
            {
                successCount++;
            }
            else
            {
                alreadyClaimedCount++;
            }
        }

        // Assert 3: Exactly 1 real insertion success, and 19 idempotent alreadyClaimed responses
        Assert.Equal(1, successCount);
        Assert.Equal(19, alreadyClaimedCount);

        // Assert 4: Exactly 1 row recorded in the underlying checkpoint store
        Assert.Equal(1, repository.TotalRecordedRows);
    }

    [Fact]
    public async Task SequentialCheckpointClaims_UpdateLatestCheckpointIndexCorrectly()
    {
        // Arrange
        var repository = new SimulatedConcurrentCheckpointRepository();
        var controller = new CheckpointsController(repository);

        // Act
        await controller.ClaimCheckpoint("season-1", "player-alpha", new ClaimCheckpointRequest(0), CancellationToken.None);
        await controller.ClaimCheckpoint("season-1", "player-alpha", new ClaimCheckpointRequest(1), CancellationToken.None);
        await controller.ClaimCheckpoint("season-1", "player-alpha", new ClaimCheckpointRequest(2), CancellationToken.None);

        var latestResult = await controller.GetLatestCheckpoint("season-1", "player-alpha", CancellationToken.None);

        // Assert
        var okResult = Assert.IsType<OkObjectResult>(latestResult);
        var latestIndex = (int)okResult.Value!.GetType().GetProperty("latestCheckpointIndex")!.GetValue(okResult.Value)!;

        Assert.Equal(2, latestIndex);
    }

    [Fact]
    public async Task ClaimCheckpoint_WithNegativeIndex_ReturnsBadRequest400()
    {
        // Arrange
        var repository = new SimulatedConcurrentCheckpointRepository();
        var controller = new CheckpointsController(repository);

        // Act
        var result = await controller.ClaimCheckpoint("season-1", "player-alpha", new ClaimCheckpointRequest(-1), CancellationToken.None);

        // Assert
        Assert.IsType<BadRequestObjectResult>(result);
    }
}
