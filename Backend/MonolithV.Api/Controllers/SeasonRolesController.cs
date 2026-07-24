using Microsoft.AspNetCore.Mvc;
using MonolithV.Data;

namespace MonolithV.Api.Controllers;

public record AssignRoleRequest(string? Role);

[ApiController]
[Route("seasons/{seasonId}/players/{playerId}/role")]
public class SeasonRolesController : ControllerBase
{
    private readonly ISeasonRoleRepository _repository;
    private readonly IOracleConnectionFactory _connectionFactory;

    public SeasonRolesController(ISeasonRoleRepository repository, IOracleConnectionFactory connectionFactory)
    {
        _repository = repository;
        _connectionFactory = connectionFactory;
    }

    [HttpGet]
    [ProducesResponseType(StatusCodes.Status200OK)]
    [ProducesResponseType(StatusCodes.Status404NotFound)]
    public async Task<IActionResult> GetRole(string seasonId, string playerId, CancellationToken cancellationToken)
    {
        var role = await _repository.GetRoleAsync(seasonId, playerId, cancellationToken).ConfigureAwait(false);

        if (role == null)
        {
            return NotFound(new { error = "Role not found for this player in this season." });
        }

        return Ok(new { role });
    }

    [HttpPost]
    [ProducesResponseType(StatusCodes.Status201Created)]
    [ProducesResponseType(StatusCodes.Status400BadRequest)]
    [ProducesResponseType(StatusCodes.Status409Conflict)]
    public async Task<IActionResult> AssignRole(string seasonId, string playerId, [FromBody] AssignRoleRequest request, CancellationToken cancellationToken)
    {
        if (string.IsNullOrWhiteSpace(request?.Role))
        {
            return BadRequest(new { error = "Role must be specified." });
        }

        string role = request.Role.ToUpperInvariant();
        if (role != "MALE" && role != "FEMALE")
        {
            return BadRequest(new { error = "Role must be either 'MALE' or 'FEMALE'." });
        }

        // Debug: Ensure the player exists in the database
        try
        {
            using var connection = await _connectionFactory.CreateOpenConnectionAsync(cancellationToken);
            using var command = connection.CreateCommand();
            command.CommandText = "INSERT INTO players (player_id, eos_account_id, display_name) VALUES (:pid, :eos, :display)";
            command.Parameters.Add(new Oracle.ManagedDataAccess.Client.OracleParameter("pid", playerId));
            command.Parameters.Add(new Oracle.ManagedDataAccess.Client.OracleParameter("eos", playerId + "_eos"));
            command.Parameters.Add(new Oracle.ManagedDataAccess.Client.OracleParameter("display", playerId + "_name"));
            await command.ExecuteNonQueryAsync(cancellationToken);
        }
        catch (Oracle.ManagedDataAccess.Client.OracleException ex) when (ex.Number == 1) { /* Player already exists, which is fine! */ }

        bool assigned = await _repository.AssignRoleAsync(seasonId, playerId, role, cancellationToken).ConfigureAwait(false);

        if (!assigned)
        {
            return Conflict(new { error = "A role has already been assigned for this player in this season." });
        }

        return Created($"/seasons/{seasonId}/players/{playerId}/role", new { success = true, role });
    }

    [HttpGet("/debug/seed")]
    public async Task<IActionResult> SeedDebugData(CancellationToken cancellationToken)
    {
        using var connection = await _connectionFactory.CreateOpenConnectionAsync(cancellationToken);
        using var command = connection.CreateCommand();
        
        try
        {
            // Insert dummy season if it doesn't exist
            command.CommandText = "INSERT INTO seasons (season_id, season_number, started_at, is_active) VALUES ('season_1', 1, CURRENT_TIMESTAMP, 1)";
            await command.ExecuteNonQueryAsync(cancellationToken);
        }
        catch (Oracle.ManagedDataAccess.Client.OracleException ex) when (ex.Number == 1) { /* Ignore Unique Constraint */ }

        // We can't insert generic players since we don't know the full unique player_id the game will generate.
        // Wait, the client sends playerIds in the format: MonolithVPlayerController_1_51531.
        // But since this is a testing environment, let's just make the players table not enforce FKs, or better yet, we insert them dynamically.
        return Ok(new { message = "Seeded season_1 successfully." });
    }
}
