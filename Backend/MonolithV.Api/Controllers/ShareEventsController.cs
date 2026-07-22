using Microsoft.AspNetCore.Mvc;
using MonolithV.Data;

namespace MonolithV.Api.Controllers;

public record CreateShareEventRequest(
    string? GiverPlayerId,
    string? ReceiverPlayerId,
    string? ItemType
);

[ApiController]
[Route("seasons/{seasonId}/share-events")]
public class ShareEventsController : ControllerBase
{
    private readonly IShareEventRepository _repository;

    public ShareEventsController(IShareEventRepository repository)
    {
        _repository = repository;
    }

    [HttpPost]
    [ProducesResponseType(StatusCodes.Status200OK)]
    [ProducesResponseType(StatusCodes.Status400BadRequest)]
    public async Task<IActionResult> CreateShareEvent(string seasonId, [FromBody] CreateShareEventRequest request, CancellationToken cancellationToken)
    {
        if (string.IsNullOrWhiteSpace(seasonId) ||
            string.IsNullOrWhiteSpace(request?.GiverPlayerId) ||
            string.IsNullOrWhiteSpace(request?.ReceiverPlayerId) ||
            string.IsNullOrWhiteSpace(request?.ItemType))
        {
            return BadRequest(new { error = "All fields (seasonId, giverPlayerId, receiverPlayerId, itemType) are required." });
        }

        if (string.Equals(request.GiverPlayerId, request.ReceiverPlayerId, StringComparison.OrdinalIgnoreCase))
        {
            return BadRequest(new { error = "Giver and receiver must be different players." });
        }

        var result = await _repository.RecordShareEventAsync(
            seasonId,
            request.GiverPlayerId,
            request.ReceiverPlayerId,
            request.ItemType,
            cancellationToken).ConfigureAwait(false);

        if (!result.Success)
        {
            return BadRequest(new { error = result.Message });
        }

        return Ok(new
        {
            success = true,
            alreadyShared = result.AlreadyShared,
            shareEventId = result.ShareEventId,
            message = result.Message
        });
    }
}
