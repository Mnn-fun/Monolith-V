using Microsoft.AspNetCore.Mvc;
using MonolithV.Data;

namespace MonolithV.Api.Controllers;

public record ClaimCheckpointRequest(int CheckpointIndex);

[ApiController]
[Route("seasons/{seasonId}/players/{playerId}/checkpoints")]
public class CheckpointsController : ControllerBase
{
    private readonly ICheckpointRepository _repository;

    public CheckpointsController(ICheckpointRepository repository)
    {
        _repository = repository;
    }

    [HttpPost]
    [ProducesResponseType(StatusCodes.Status200OK)]
    [ProducesResponseType(StatusCodes.Status400BadRequest)]
    public async Task<IActionResult> ClaimCheckpoint(string seasonId, string playerId, [FromBody] ClaimCheckpointRequest request, CancellationToken cancellationToken)
    {
        if (string.IsNullOrWhiteSpace(seasonId) || string.IsNullOrWhiteSpace(playerId) || request is null)
        {
            return BadRequest(new { error = "SeasonId, PlayerId, and CheckpointIndex are required." });
        }

        if (request.CheckpointIndex < 0)
        {
            return BadRequest(new { error = "Checkpoint index must be non-negative." });
        }

        var result = await _repository.RecordCheckpointProgressAsync(
            seasonId,
            playerId,
            request.CheckpointIndex,
            cancellationToken).ConfigureAwait(false);

        if (!result.Success)
        {
            return BadRequest(new { error = result.Message });
        }

        return Ok(new
        {
            success = true,
            alreadyClaimed = result.AlreadyClaimed,
            checkpointIndex = request.CheckpointIndex,
            message = result.Message
        });
    }

    [HttpGet("latest")]
    [ProducesResponseType(StatusCodes.Status200OK)]
    [ProducesResponseType(StatusCodes.Status400BadRequest)]
    public async Task<IActionResult> GetLatestCheckpoint(string seasonId, string playerId, CancellationToken cancellationToken)
    {
        if (string.IsNullOrWhiteSpace(seasonId) || string.IsNullOrWhiteSpace(playerId))
        {
            return BadRequest(new { error = "SeasonId and PlayerId are required." });
        }

        var latestIndex = await _repository.GetLatestCheckpointIndexAsync(seasonId, playerId, cancellationToken).ConfigureAwait(false);

        return Ok(new
        {
            playerId = playerId,
            seasonId = seasonId,
            latestCheckpointIndex = latestIndex
        });
    }
}
