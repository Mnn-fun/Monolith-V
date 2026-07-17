# Project Behavioral Rules

## Git Commit Formatting
- **No Prompt Numbers in Commit Messages**: Never include prompt IDs or numbers (e.g., `[P1.5]`, `[P2.1]`) in Git commit messages.
- Always follow standard **Conventional Commits** formatting (e.g., `feat: scaffold ASP.NET Core backend solution with health endpoint`, `fix: ...`, `docs: ...`).

## Twitter / X Social Posts
- **No Prompt Numbers in Tweets**: Never include prompt numbers or internal prompt identifiers when suggesting or drafting Twitter/X posts.
- **On-Demand Posting Only**: Do not automatically prompt or generate Twitter/X post drafts at prompt boundaries or minor updates. Only prepare or suggest posts when there are major, substantial project updates or when the user explicitly expresses the need to post.
- **No Auto-Pushing**: Never run `git push` autonomously. The user will review and push when satisfied.

## Post-Prompt Verification & Testing Instructions
- **Always Provide Testing Commands**: At the end of completing every prompt or major task, explicitly include a **Testing & Verification** section in the final response.
- **Copy-Pasteable Commands**: Provide exact terminal commands (`dotnet test`, `dotnet run`, `curl`, `stat net`, etc.) that the user can run directly in their shell.
- **Expected Outcome**: Clearly describe what successful behavior looks like (e.g., "HTTP 200 OK with `status: healthy`", "0 errors in build log", "Character pawn spawns with replication enabled").
- **Manual Checklists**: If a check requires visual or in-editor inspection (like checking Unreal Editor logs or PIE behavior), list exact, concise manual verification steps.

## Cloud Infrastructure & State Tracking
- **Active OCI Compute Instance (`ap-mumbai-1`)**: The user has successfully launched the Always-Free `VM.Standard.A1.Flex` (2 OCPUs / 12 GB RAM, Canonical Ubuntu 22.04 LTS) instance (`MonolithV-instance`) in OCI Console (`ap-mumbai-1`). Local reference scripts (`provision-vm.sh`) and wallet structure (`Infra/oracle/README.md`) are prepped. Next steps involve live SSH verification and schema deployment in P1.7.
