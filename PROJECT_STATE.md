# Project State

> This file is the single source of truth for "where is this project right now." Any AI coding agent (Claude, Antigravity, GitHub Copilot, etc.) picking up this project cold should read this file FIRST, before opening any prompt file.

**Last Completed Prompt ID:** P1.4
**Current Phase:** Phase 1 — Project Foundation
**Last Updated:** 2026-07-07
**Last Updated By:** Claude Code

## Next Prompt To Run
`P1.5` in `PromptBook/01_Project_Foundation.md`

## Build Status
- Unreal client: scaffolded, compiles (see path note below); GameMode/Character stubs added, 30Hz server tick configured; EOS subsystem confirmed live (`LogOnline: OSS: Created online subsystem instance for: EOS` / `Loaded subsystem for type [EOS]`, no errors)
- Dedicated server build: not yet created
- Backend (ASP.NET): not yet created
- Database (Oracle): not yet provisioned
- Redis: not yet provisioned
- CI: not yet configured

## Files Touched So Far
- `game/Monolith_V/` — UE5 project scaffold (`Monolith_V.uproject`, `MonolithV` runtime module, `Source/MonolithV/{Player,Combat,AI,World,Networking,UI}/` placeholder headers)
- `game/Monolith_V/Source/MonolithV/MonolithVGameMode.h/.cpp` — `AGameModeBase` stub, `DefaultPawnClass = AMonolithVCharacter`
- `game/Monolith_V/Source/MonolithV/Player/MonolithVCharacter.h/.cpp` — `ACharacter` stub, `bReplicates = true` (replaces `PlaceholderPlayer.h`)
- `game/Monolith_V/Config/DefaultEngine.ini` — `GlobalDefaultGameMode`, `NetServerMaxTickRate=30`, player speed config, `[OnlineSubsystem] DefaultPlatformService=EOS`
- `game/Monolith_V/Config/UserEngine.ini` (new, gitignored) — full `[/Script/OnlineSubsystemEOS.EOSSettings]` `Artifacts` entry with real dev credentials (never committed). Note: this is the engine's real per-user override file for Engine-type config (`ConfigHierarchy.h`'s `GameDirUser` layer); an earlier attempt used the invented, never-loaded name `DefaultEngine.Local.ini` — deleted.
- `game/Monolith_V/Source/MonolithV/MonolithV.cpp` — startup log confirming `IOnlineSubsystem::Get(TEXT("EOS"))` non-null
- `Docs/Architecture/NetworkingNotes.md` (new)
- `Docs/InstallationGuide.md` (new) — EOS setup section

## Known Issues / Open TODOs
- Path/naming drift: PromptBook references `Game/MonolithV.uproject`; actual path is `game/Monolith_V/Monolith_V.uproject` (lowercase folder, underscore in project name). Not fixed — later prompts should use the actual path, not the literal path in the prompt text.
- P1.4 debugging trail (both root-caused via the actual UE 5.7 engine source, not guessed): (1) `GetSelectedArtifactSettings failed` — the `Artifacts` entry needs empty `ArtifactName` and the field is `ClientEncryptionKey` not `EncryptionKey`; (2) credentials weren't loading at all because `DefaultEngine.Local.ini` isn't a real config-hierarchy file — real per-user override is `Config/UserEngine.ini`. Both fixed; confirmed live via `LogOnline: OSS: Created online subsystem instance for: EOS`.
- The custom `MonolithV.cpp` startup log (`EOS Subsystem: OK`) has never actually printed across any test, likely because Live Coding's patch never applied ("Client communication broken, patch could not be loaded") and Live Coding patches don't persist into the base module DLL for a later cold start — only a real Build Solution does. Not blocking (engine's own logs already prove the subsystem works), but if that line matters later, do a real Visual Studio rebuild, not Live Coding/editor-restart-only.

## Environment Notes
_(record local vs cloud connection strings, ports, credentials locations — NEVER commit actual secrets here, only where they are stored, e.g. ".env.local (gitignored)")_

---

### How to update this file
At the end of every prompt, replace the fields above with the new state, append nothing — this file always reflects only the CURRENT state, not a history (git log is the history). Commit this file's update in the same commit as the prompt's other changes.
