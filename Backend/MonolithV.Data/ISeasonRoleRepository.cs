using System.Threading;
using System.Threading.Tasks;

namespace MonolithV.Data;

public interface ISeasonRoleRepository
{
    Task<string?> GetRoleAsync(string seasonId, string playerId, CancellationToken cancellationToken);
    Task<bool> AssignRoleAsync(string seasonId, string playerId, string role, CancellationToken cancellationToken);
}
