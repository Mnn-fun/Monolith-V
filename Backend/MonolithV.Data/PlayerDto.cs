namespace MonolithV.Data;

/// <summary>
/// Immutable data transfer object representing a core player record in Oracle (PLAYERS table).
/// </summary>
public record PlayerDto(
    string PlayerId,
    string EosAccountId,
    string DisplayName,
    DateTimeOffset CreatedAt
);
