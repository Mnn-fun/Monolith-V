# Project State

> This file is the single source of truth for "where is this project right now." Any AI coding agent (Claude, Antigravity, GitHub Copilot, etc.) picking up this project cold should read this file FIRST, before opening any prompt file.

**Last Completed Prompt ID:** P2.3
**Current Phase:** Phase 2 — Core Architecture (Completed P2.3)
**Last Updated:** 2026-07-21
**Last Updated By:** Antigravity

## Next Prompt To Run
`P2.4` in `PromptBook/02_Core_Architecture.md`

## Build Status
- Unreal client: scaffolded, compiles offline and links cleanly (`UnrealEditor-MonolithV.dll`); GameMode overrode `SpawnDefaultPawnFor_Implementation` for safe side-by-side offset spawning; `AMonolithVCharacter` wired with Enhanced Input (`IA_Move`, `IA_Look`, `IMC_Default`), replicated `Health` / `OnRep_Health`, `NetUpdateFrequency=30.f`, `MinNetUpdateFrequency=10.f`, `NetworkSmoothingMode=Exponential`, and visible TPP Cylinder avatar (`/Engine/BasicShapes/Cylinder.Cylinder`, `500.0f` spring arm); `AMonolithVPlayerController` implemented with `TestDamage` execution command. EOS subsystem confirmed live (`LogOnline: OSS: Created online subsystem instance for: EOS` / `Loaded subsystem for type [EOS]`).
- Dedicated server build: not yet created
- Backend (ASP.NET): scaffolded (`MonolithV.Backend.sln` with `Api`, `Data`, `Tests`), compiles cleanly with zero errors. `GET /health` confirmed responding `200 OK` (`{"status":"healthy",...}`). `GET /players/{eosAccountId}` endpoint wired to `PlayerRepository` using non-blocking async `OracleConnectionFactory` and strict bind variables (`:eosAccountId`). xUnit test suite (`MonolithV.Tests`) passing (`5 passed, 0 failed`).
- Database (Oracle): local documentation (`Infra/oracle/README.md`), `.gitignore` wallet rules, and V1 DDL schema migration script (`Infra/oracle/schema/V1__init_schema.sql`) live in cloud on `Autonomous Transaction Processing` (`monolithvdb`). ER Diagram documented at `Docs/Architecture/ER_Diagram.md`.
- Cloud Compute VM: active (`VM.Standard.A1.Flex`, 2 OCPUs, 12 GB RAM, Ubuntu 22.04 LTS, `MonolithV-instance` live in `ap-mumbai-1`); reference script `Infra/cloud/provision-vm.sh` created.
- Redis: not yet provisioned
- CI: not yet configured

## Files Touched So Far
- `game/Monolith_V/` — UE5 project scaffold (`Monolith_V.uproject`, `MonolithV` runtime module, `Source/MonolithV/{Player,Combat,AI,World,Networking,UI}/` headers)
- `game/Monolith_V/Source/MonolithV/MonolithVGameMode.h/.cpp` — `AGameModeBase` stub, `DefaultPawnClass = AMonolithVCharacter`, `PlayerControllerClass = AMonolithVPlayerController`, `SpawnDefaultPawnFor_Implementation` side-by-side offset spawning (`Z+150`, `X separated by 200 units`)
- `game/Monolith_V/Source/MonolithV/Player/MonolithVCharacter.h/.cpp` — `ACharacter` with Enhanced Input binding (`IA_Move`, `IA_Look`, `IMC_Default`), server-authoritative movement (`MaxWalkSpeed=600`, `GravityScale=1.0`), `NetUpdateFrequency=30.f`, `MinNetUpdateFrequency=10.f`, `NetworkSmoothingMode=Exponential`, replicated `Health` / `OnRep_Health`, 3rd-person camera boom (`TargetArmLength=500.0f`, `SocketOffset Z=80.0f`), and replicated `VisualMesh` (`/Engine/BasicShapes/Cylinder.Cylinder`)
- `game/Monolith_V/Source/MonolithV/Player/MonolithVPlayerController.h/.cpp` — `APlayerController` with `TestDamage` execution command (`UFUNCTION(Server, Reliable, WithValidation)`) calling `TakeDamage` on possessed pawn
- `Docs/Testing/TestPlan.md` — added Phase 2 `"Networking — Tick Rate & Smoothing Verification"` section skeleton (`P2.3`)
- `Docs/Architecture/Sequence_Diagram.md` — Mermaid sequence diagrams detailing Server-Authoritative character movement (`AddMovementInput`) and server damage replication (`TestDamage`)
- `PromptBook/twitter_roadmap.md` — Migrated to `PromptBook/` and updated with short copy (<250 characters, no prompt/phase IDs, subphase + post number markers)
- `game/Monolith_V/Config/DefaultEngine.ini` — `GlobalDefaultGameMode=/Script/MonolithV.MonolithVGameMode`, `NetServerMaxTickRate=30`, `[OnlineSubsystem] DefaultPlatformService=EOS`
- `game/Monolith_V/Config/UserEngine.ini` (gitignored) — full `[/Script/OnlineSubsystemEOS.EOSSettings]` `Artifacts` entry with dev credentials
- `game/Monolith_V/Source/MonolithV/MonolithV.cpp` — startup log confirming `IOnlineSubsystem::Get(TEXT("EOS"))` non-null
- `Backend/MonolithV.Backend.sln` — ASP.NET Core solution containing `Api`, `Data`, and `Tests` projects
- `Backend/MonolithV.Api/` — REST endpoints project (`Program.cs`, `HealthController.cs`, `PlayersController.cs`, `appsettings.Development.json.example`)
- `Backend/MonolithV.Data/` — Data access class library (`MonolithV.Data.csproj` with `Oracle.ManagedDataAccess.Core` and `Microsoft.Extensions.Configuration.Abstractions`, `PlayerDto.cs`, `OracleConnectionFactory.cs`, `PlayerRepository.cs`)
- `Backend/MonolithV.Tests/` — xUnit test suite (`MonolithV.Tests.csproj`, `HealthControllerTests.cs`, `PlayerRepositoryAndControllerTests.cs`), running cleanly
- `Docs/Architecture/NetworkingNotes.md`
- `Docs/Architecture/Components.md` — Deployment Target specification note (OCI Free Tier, Ampere ARM VM, Autonomous Database `_medium`)
- `Docs/InstallationGuide.md` — EOS setup section
- `.gitignore` — updated with `Infra/oracle/wallet/` and `*.zip` exclusions
- `Infra/cloud/provision-vm.sh` — documented OCI CLI reference script for compute and VCN provisioning (`A1.Flex`, `2 OCPUs`, `12 GB RAM`)
- `Infra/oracle/README.md` — Oracle Autonomous Database wallet directory setup, TNS alias guide (`_medium`), and environment variable reference
- `Infra/oracle/schema/V1__init_schema.sql` — V1 DDL schema creation script (`PLAYERS`, `SEASONS`, `PLAYER_SEASON_ROLES`, `SHARE_EVENTS`, `CHECKPOINT_PROGRESS`)
- `Docs/Architecture/ER_Diagram.md` — Mermaid `erDiagram` block and table constraint definitions

## Known Issues / Open TODOs
- Path/naming drift: PromptBook references `Game/MonolithV.uproject`; actual path is `game/Monolith_V/Monolith_V.uproject` (lowercase folder, underscore in project name).
- UHT & Live Coding Patching: When adding new `CreateDefaultSubobject` subcomponents or modifying header `.h` properties (`UPROPERTY`), Live Coding (`Ctrl+Alt+F11`) cannot rerun C++ constructors on existing Class Default Objects (CDOs) or regenerate UHT network replication indices (`ValidateGeneratedRepEnums()`). Always perform an offline build (`Build.bat` or rebuild from project launch) when modifying headers or constructors.

## Environment Notes
_(record local vs cloud connection strings, ports, credentials locations — NEVER commit actual secrets here, only where they are stored, e.g. ".env.local (gitignored)")_

---

### How to update this file
At the end of every prompt, replace the fields above with the new state, append nothing — this file always reflects only the CURRENT state, not a history (git log is the history). Commit this file's update in the same commit as the prompt's other changes.
