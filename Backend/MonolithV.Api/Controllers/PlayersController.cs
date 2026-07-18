using Microsoft.AspNetCore.Mvc;
using MonolithV.Data;

namespace MonolithV.Api.Controllers;

[ApiController]
[Route("players")]
public class PlayersController : ControllerBase
{
    private readonly IPlayerRepository _playerRepository;

    public PlayersController(IPlayerRepository playerRepository)
    {
        _playerRepository = playerRepository;
    }

    [HttpGet("{eosAccountId}")]
    [ProducesResponseType(typeof(PlayerDto), StatusCodes.Status200OK)]
    [ProducesResponseType(StatusCodes.Status404NotFound)]
    [ProducesResponseType(StatusCodes.Status400BadRequest)]
    public async Task<IActionResult> GetByEosAccountId(string eosAccountId, CancellationToken cancellationToken)
    {
        if (string.IsNullOrWhiteSpace(eosAccountId))
        {
            return BadRequest("EOS Account ID must not be empty.");
        }

        var player = await _playerRepository.GetByEosAccountIdAsync(eosAccountId, cancellationToken).ConfigureAwait(false);
        if (player is null)
        {
            return NotFound();
        }

        return Ok(player);
    }
}
