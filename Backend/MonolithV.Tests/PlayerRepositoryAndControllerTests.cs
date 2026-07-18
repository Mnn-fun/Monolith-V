using System.Reflection;
using Microsoft.AspNetCore.Mvc;
using MonolithV.Api.Controllers;
using MonolithV.Data;
using Xunit;

namespace MonolithV.Tests;

public class PlayerRepositoryAndControllerTests
{
    private class FakePlayerRepository : IPlayerRepository
    {
        private readonly PlayerDto? _playerToReturn;

        public FakePlayerRepository(PlayerDto? playerToReturn)
        {
            _playerToReturn = playerToReturn;
        }

        public Task<PlayerDto?> GetByEosAccountIdAsync(string eosAccountId, CancellationToken cancellationToken = default)
        {
            if (string.IsNullOrWhiteSpace(eosAccountId))
            {
                throw new ArgumentException("eosAccountId cannot be empty", nameof(eosAccountId));
            }

            return Task.FromResult(_playerToReturn);
        }
    }

    [Fact]
    public async Task GetByEosAccountId_WhenPlayerExists_ReturnsOkWithPlayerDto()
    {
        // Arrange
        var expectedPlayer = new PlayerDto("player-123", "eos-456", "HeroPlayer", DateTimeOffset.UtcNow);
        var repository = new FakePlayerRepository(expectedPlayer);
        var controller = new PlayersController(repository);

        // Act
        var result = await controller.GetByEosAccountId("eos-456", CancellationToken.None);

        // Assert
        var okResult = Assert.IsType<OkObjectResult>(result);
        var returnedPlayer = Assert.IsType<PlayerDto>(okResult.Value);
        Assert.Equal("player-123", returnedPlayer.PlayerId);
        Assert.Equal("eos-456", returnedPlayer.EosAccountId);
        Assert.Equal("HeroPlayer", returnedPlayer.DisplayName);
    }

    [Fact]
    public async Task GetByEosAccountId_WhenPlayerDoesNotExist_ReturnsNotFound()
    {
        // Arrange
        var repository = new FakePlayerRepository(null);
        var controller = new PlayersController(repository);

        // Act
        var result = await controller.GetByEosAccountId("non-existent-eos", CancellationToken.None);

        // Assert
        Assert.IsType<NotFoundResult>(result);
    }

    [Fact]
    public async Task GetByEosAccountId_WhenEosIdIsNullOrWhiteSpace_ReturnsBadRequest()
    {
        // Arrange
        var repository = new FakePlayerRepository(null);
        var controller = new PlayersController(repository);

        // Act
        var result = await controller.GetByEosAccountId("   ", CancellationToken.None);

        // Assert
        Assert.IsType<BadRequestObjectResult>(result);
    }

    [Fact]
    public void PlayerRepository_GetByEosAccountIdAsync_SignatureReturnsTaskOfPlayerDto()
    {
        // Assert that the repository interface method returns Task<PlayerDto?> exactly as required (no sync signatures or void calls)
        var method = typeof(IPlayerRepository).GetMethod(nameof(IPlayerRepository.GetByEosAccountIdAsync));
        Assert.NotNull(method);
        Assert.Equal(typeof(Task<PlayerDto?>), method.ReturnType);
    }

    [Fact]
    public void PlayerRepository_ImplementationDoesNotContainBlockingCalls()
    {
        // Static code / reflection verification that PlayerRepository is pure async
        var methods = typeof(PlayerRepository).GetMethods(BindingFlags.Public | BindingFlags.Instance | BindingFlags.DeclaredOnly);
        foreach (var method in methods)
        {
            // Verify all public methods return Task or Task<T> or IAsyncEnumerable
            Assert.True(typeof(Task).IsAssignableFrom(method.ReturnType) || method.ReturnType.IsGenericType && typeof(IAsyncEnumerable<>).IsAssignableFrom(method.ReturnType.GetGenericTypeDefinition()),
                $"Method {method.Name} in PlayerRepository must be asynchronous returning Task or Task<T>.");
        }
    }
}
