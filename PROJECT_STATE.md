# Project State

> This file is the single source of truth for "where is this project right now." Any AI coding agent (Claude, Antigravity, GitHub Copilot, etc.) picking up this project cold should read this file FIRST, before opening any prompt file.

**Last Completed Prompt ID:** P1.3
**Current Phase:** Phase 1 — Project Foundation
**Last Updated:** 2026-07-07
**Last Updated By:** Claude Code

## Next Prompt To Run
`P1.4` in `PromptBook/01_Project_Foundation.md`

## Build Status
- Unreal client: scaffolded, compiles (see path note below); GameMode/Character stubs added, 30Hz server tick configured
- Dedicated server build: not yet created
- Backend (ASP.NET): not yet created
- Database (Oracle): not yet provisioned
- Redis: not yet provisioned
- CI: not yet configured

## Files Touched So Far
- `game/Monolith_V/` — UE5 project scaffold (`Monolith_V.uproject`, `MonolithV` runtime module, `Source/MonolithV/{Player,Combat,AI,World,Networking,UI}/` placeholder headers)
- `game/Monolith_V/Source/MonolithV/MonolithVGameMode.h/.cpp` — `AGameModeBase` stub, `DefaultPawnClass = AMonolithVCharacter`
- `game/Monolith_V/Source/MonolithV/Player/MonolithVCharacter.h/.cpp` — `ACharacter` stub, `bReplicates = true` (replaces `PlaceholderPlayer.h`)
- `game/Monolith_V/Config/DefaultEngine.ini` — `GlobalDefaultGameMode`, `NetServerMaxTickRate=30`, player speed config
- `Docs/Architecture/NetworkingNotes.md` (new)

## Known Issues / Open TODOs
- Path/naming drift: PromptBook references `Game/MonolithV.uproject`; actual path is `game/Monolith_V/Monolith_V.uproject` (lowercase folder, underscore in project name). Not fixed — later prompts should use the actual path, not the literal path in the prompt text.

## Environment Notes
_(record local vs cloud connection strings, ports, credentials locations — NEVER commit actual secrets here, only where they are stored, e.g. ".env.local (gitignored)")_

---

### How to update this file
At the end of every prompt, replace the fields above with the new state, append nothing — this file always reflects only the CURRENT state, not a history (git log is the history). Commit this file's update in the same commit as the prompt's other changes.
