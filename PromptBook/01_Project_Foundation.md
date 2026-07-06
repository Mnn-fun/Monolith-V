# Phase 1 — Project Foundation

Goal of this phase: an empty computer becomes a fully scaffolded, version-controlled, CI-checked repository with an Unreal project, a backend solution, a provisioned database and cache, and the skeleton of the academic documentation — nothing playable yet, but every system has a real, running "hello world."

Each prompt below is self-contained and tool-agnostic. Paste one prompt at a time into whatever AI coding agent you currently have available.

---

## P1.1 — Initialize Repository & GitFlow Branching

**Prerequisites / State Check:** `PROJECT_STATE.md` shows `Last Completed Prompt ID: none`. The folder contains only `README.md` and `PROJECT_STATE.md`. If a `.git` folder already exists, run `git status` and stop to inspect before proceeding — do not overwrite existing history.

**Objective:** Turn the project folder into a Git repository following full GitFlow, with a `.gitignore` appropriate for Unreal Engine + .NET, and push it to a new public GitHub repository named `Monolith-V`.

**Expected Output:** A GitHub repo with `main` and `develop` branches, GitFlow conventions documented, first commit containing `README.md`, `PROJECT_STATE.md`, and `.gitignore`.

**Files to Modify:**
- `.gitignore` (new)
- `.github/CONTRIBUTING.md` (new, brief GitFlow note for your own future reference)

**Implementation Steps:**
1. `git init` in the project root.
2. Create `.gitignore` with entries for: Unreal (`Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, `.vs/`, `*.sln` for the uproject-generated solution — but NOT the hand-written `Backend/*.sln`), .NET (`bin/`, `obj/`), and secrets (`.env.local`, `appsettings.Development.json` if it will hold real credentials).
3. `git add README.md PROJECT_STATE.md .gitignore` and commit.
4. Create branch `develop` from `main`: `git checkout -b develop`.
5. On GitHub, create a new **public** repository `Monolith-V` (no README/gitignore auto-init, since you already have one locally).
6. `git remote add origin <your-repo-url>`, push both `main` and `develop`.
7. In GitHub repo settings, set `develop` as the default branch for day-to-day PRs, keep `main` protected (require the CI check from P1.10 once it exists — revisit after that prompt).
8. Write `.github/CONTRIBUTING.md`: one short paragraph stating the branch model — `feature/<name>` branches off `develop`, PR back into `develop`; `release/<version>` branches off `develop` when a phase completes, merges into both `main` and `develop`.

**Validation Checklist:**
- [ ] `git branch` shows both `main` and `develop`
- [ ] GitHub repo is public and reachable
- [ ] `.gitignore` correctly ignores `Binaries/`, `Intermediate/`, `Saved/`, `bin/`, `obj/`
- [ ] No secrets committed (double-check `git log -p` on the first commit)

**Testing Checklist:**
- [ ] Clone the repo fresh into a temp folder and confirm both branches and all 3 files are present

**Git Commit Message:**
```
[P1.1] Initialize repository with GitFlow branching and gitignore
```

**Documentation Updates:** None yet (SRS/SDLC come in P1.12/P1.13).

**Common Mistakes:**
- Forgetting to ignore `Saved/` — this folder contains local editor state and grows large fast.
- Committing `appsettings.Development.json` with a real Oracle/Redis connection string before P1.6/P1.9 exist — keep a `.example` version instead once those prompts add real config.

**Next Prompt Dependency:** P1.2 requires `develop` branch to exist and be checked out.

---

## P1.2 — Scaffold the Unreal Engine 5 C++ Project

**Prerequisites / State Check:** On `develop` branch. `PROJECT_STATE.md` → `Last Completed Prompt ID: P1.1`.

**Objective:** Create the Unreal Engine 5 project `MonolithV` as a C++ project (not Blueprint-only) with the module layout defined in the README's folder structure, targeting both a standard game client and a dedicated server build target.

**Expected Output:** A `Game/MonolithV.uproject` that opens in UE5, compiles, and runs an empty default map, with the `MonolithV` C++ module present and the folder skeleton for `Player/`, `Combat/`, `AI/`, `World/`, `Networking/`, `UI/` created (each with a placeholder `.gitkeep` or a trivial header).

**Files to Modify:**
- `Game/MonolithV.uproject` (new)
- `Game/Source/MonolithV/MonolithV.Build.cs` (new)
- `Game/Source/MonolithV/MonolithV.cpp`, `MonolithV.h` (new, module entry point)
- `Game/Source/MonolithVServer/MonolithVServer.Target.cs` (new, dedicated server target)
- `Game/Source/MonolithV/MonolithV.Target.cs` and `MonolithVEditor.Target.cs` (new, standard targets)
- `Game/Config/DefaultEngine.ini`, `DefaultGame.ini` (new)

**Implementation Steps:**
1. In Epic Games Launcher / UE5 project browser, create a new **Games > Third Person (or Blank) > C++** project named `MonolithV`, location = `Game/` inside this repo.
2. Delete any starter-content assets you don't intend to keep long-term (keep the project lean).
3. Open `Game/Source/MonolithV/MonolithV.Build.cs`, confirm it depends on at minimum: `Core`, `CoreUObject`, `Engine`, `InputCore`, `GameplayAbilities`, `GameplayTags`, `GameplayTasks`, `OnlineSubsystem`, `OnlineSubsystemUtils`, `HTTP`, `Json`, `JsonUtilities`.
4. Add a `MonolithVServer.Target.cs` alongside the existing `MonolithV.Target.cs`, copying its structure but setting `Type = TargetType.Server;` and `TargetName = "MonolithVServer"` — this is what produces the headless Linux dedicated server binary later.
5. Create the subfolders `Source/MonolithV/Player/`, `Combat/`, `AI/`, `World/`, `Networking/`, `UI/`, each containing a one-line placeholder header (e.g. `Player/PlaceholderPlayer.h` with just a comment) so Git tracks the empty folders — replace these as real classes land in Phase 2/3.
6. Set `Game/Config/DefaultGame.ini` project name/description fields.
7. Build the project (Development Editor config) and confirm it opens in the UE5 editor with zero compile errors.
8. Commit.

**Validation Checklist:**
- [ ] `MonolithV.uproject` opens without a "missing modules, rebuild?" prompt after a clean build
- [ ] Editor launches into an empty default map
- [ ] `MonolithVServer` target exists in the `.uproject`'s target list and compiles via `RunUAT BuildCookRun` (dry-run compile only at this stage; full server packaging comes in Phase 5)
- [ ] Folder skeleton matches the README structure exactly

**Testing Checklist:**
- [ ] Package a Development Editor build with zero errors/warnings related to missing modules
- [ ] Launch standalone game (`-game` flag) and confirm it loads the default map

**Git Commit Message:**
```
[P1.2] Scaffold Unreal Engine 5 C++ project with client/server targets
```

**Documentation Updates:** Add a one-paragraph "Engine Setup" note to `Docs/InstallationGuide.md` (create the file now with just this section — it will grow through later phases).

**Common Mistakes:**
- Creating the project as Blueprint-only and converting later — always start C++ from `New Project` so the `Source/` folder and `.Build.cs` exist correctly from the start.
- Forgetting the `GameplayAbilities` module dependency now — adding it later requires a re-generate of project files and is easy to forget, causing confusing GAS compile errors in Phase 2.
- Not adding the server target now — retrofitting a dedicated server target after gameplay code exists is more error-prone than starting with it present (even unused) from day one.

**Next Prompt Dependency:** P1.3 configures engine settings on top of this scaffold.

---

## P1.3 — Configure Base Engine & Networking Settings

**Prerequisites / State Check:** `PROJECT_STATE.md` → `P1.2` complete. `Game/MonolithV.uproject` compiles.

**Objective:** Set the project-wide configuration that every later networking prompt depends on: fixed server tick rate, default game mode/pawn stubs, and basic project settings (frame rate caps, network settings) so Phase 2's replication work has a correct baseline instead of engine defaults.

**Expected Output:** `DefaultEngine.ini` contains an explicit `[/Script/OnlineSubsystemUtils.IpNetDriver]` section fixing `NetServerMaxTickRate=30`, plus a minimal `AMonolithVGameMode` / `AMonolithVCharacter` C++ stub wired as project defaults.

**Files to Modify:**
- `Game/Config/DefaultEngine.ini`
- `Game/Source/MonolithV/MonolithVGameMode.h/.cpp` (new)
- `Game/Source/MonolithV/Player/MonolithVCharacter.h/.cpp` (new, replaces earlier placeholder)

**Implementation Steps:**
1. Create `AMonolithVGameMode : public AGameModeBase` — empty for now beyond a constructor setting `DefaultPawnClass`.
2. Create `AMonolithVCharacter : public ACharacter` — empty beyond enabling `bReplicates = true` in the constructor (this is the seed for Phase 2/3's real character work).
3. In `DefaultEngine.ini`, under `[/Script/EngineSettings.GeneralProjectSettings]` confirm project name. Add:
   ```
   [/Script/OnlineSubsystemUtils.IpNetDriver]
   NetServerMaxTickRate=30

   [/Script/Engine.Player]
   ConfiguredInternetSpeed=100000
   ConfiguredLanSpeed=100000
   ```
4. Set the default GameMode for the default map to `AMonolithVGameMode` via World Settings in the editor (this writes into the map itself, not ini — confirm it's set).
5. Confirm `DefaultPawnClass` on the game mode points at `AMonolithVCharacter`.
6. Compile, launch standalone, confirm the character spawns and `bReplicates` doesn't throw warnings in the Output Log.
7. Commit.

**Validation Checklist:**
- [ ] `NetServerMaxTickRate=30` present and confirmed via `Net ServerTickRate` in an in-editor console check (Play-in-Editor as Listen Server, run `stat net`)
- [ ] Game mode/pawn stubs compile with no warnings
- [ ] Output Log shows no replication warnings on PIE launch

**Testing Checklist:**
- [ ] Play-in-Editor as "Listen Server" with 2 clients, confirm both pawns spawn (movement/replication logic itself is Phase 2 — just confirm no crashes)

**Git Commit Message:**
```
[P1.3] Configure base engine settings and 30Hz server tick baseline
```

**Documentation Updates:** Add "Networking Baseline: 30Hz server tick, client prediction planned for Phase 2" to `Docs/Architecture/` notes (create `Docs/Architecture/NetworkingNotes.md` as a running scratch file — folds into the sequence diagram later).

**Common Mistakes:**
- Setting tick rate only in `DefaultEngine.ini` but not verifying it actually took effect — engine defaults silently override misplaced ini sections; always verify with `stat net` or console `NetTickRate`.
- Forgetting `bReplicates = true` on the character constructor — without it nothing in Phase 2 will replicate no matter how correctly written.

**Next Prompt Dependency:** P1.4 (EOS) does not technically depend on this, but should be done next to keep the engine-side foundation contiguous before switching to backend work in P1.5+.

---

## P1.4 — Integrate Epic Online Services (EOS) Skeleton

**Prerequisites / State Check:** `P1.3` complete, project compiles with `OnlineSubsystem`/`OnlineSubsystemUtils` already in `MonolithV.Build.cs` from P1.2.

**Objective:** Enable the EOS plugin, register a development application on the Epic Developer Portal, and wire the minimum config so the game can call `IOnlineSubsystem::Get()` and receive a valid EOS interface — no real login flow yet (that's P2.8), just proving the plugin and credentials are wired correctly.

**Expected Output:** `DefaultEngine.ini` has a working `[OnlineSubsystem]` / `[OnlineSubsystemEOS]` config block with real (dev sandbox) Product ID/Sandbox ID/Deployment ID/Client Credentials, and a debug log line on startup confirming `IOnlineSubsystem::Get(EOS_SUBSYSTEM)` returns non-null.

**Files to Modify:**
- `Game/MonolithV.uproject` (enable `OnlineSubsystemEOS` plugin)
- `Game/Config/DefaultEngine.ini`
- `Game/Source/MonolithV/MonolithV.cpp` (startup log check)

**Implementation Steps:**
1. Go to the Epic Games Developer Portal, create an Organization (if you don't have one) and a new Product called "Monolith-V".
2. Under that product, create a Sandbox and a Deployment (defaults are fine for dev), and a Client (note: Product ID, Sandbox ID, Deployment ID, Client ID, Client Secret).
3. In UE5 Editor, Edit > Plugins, enable "Online Subsystem EOS" (and "Online Subsystem" if not already on), restart editor.
4. In `Game/MonolithV.uproject`'s `"Plugins"` array, confirm `"OnlineSubsystemEOS"` entry now exists with `"Enabled": true`.
5. In `DefaultEngine.ini`, add:
   ```
   [OnlineSubsystem]
   DefaultPlatformService=EOS

   [OnlineSubsystemEOS]
   bEnabled=true
   ArtifactId=MonolithV
   ProductId=<your Product ID>
   SandboxId=<your Sandbox ID>
   DeploymentId=<your Deployment ID>
   ClientId=<your Client ID>
   ClientSecret=<your Client Secret>
   ```
   Store the actual secret values in a local, gitignored `Game/Config/DefaultEngine.Local.ini` override instead of committing them directly — reference this pattern, since Unreal supports per-user `.ini` overrides.
6. In `MonolithV.cpp`'s `StartupModule()`, add a temporary debug log: `UE_LOG(LogTemp, Warning, TEXT("EOS Subsystem: %s"), IOnlineSubsystem::Get(TEXT("EOS")) ? TEXT("OK") : TEXT("NULL"));`
7. Launch the editor, confirm the log prints `OK`.
8. Remove or downgrade the debug log to `Verbose` before committing (keep it — it's useful — just not spamming `Warning` level long-term).
9. Commit (excluding the local secret override file).

**Validation Checklist:**
- [ ] `IOnlineSubsystem::Get(TEXT("EOS"))` returns non-null on startup
- [ ] No plugin-related errors in the Output Log
- [ ] Secrets are NOT in the committed `DefaultEngine.ini` — confirm via `git show` on the commit

**Testing Checklist:**
- [ ] Fresh clone + fresh `DefaultEngine.Local.ini` (recreated manually, not from git) still successfully initializes EOS

**Git Commit Message:**
```
[P1.4] Integrate Epic Online Services subsystem skeleton
```

**Documentation Updates:** `Docs/InstallationGuide.md` — add "EOS Setup" section explaining a new developer needs their own Epic Dev Portal credentials in `DefaultEngine.Local.ini` (with the exact keys expected, no real values).

**Common Mistakes:**
- Committing real Client Secret to git — this is a real, revocable credential; treat it like a password.
- Confusing "Sandbox" with "Deployment" — both IDs are required and are different concepts in the EOS dashboard.

**Next Prompt Dependency:** P1.5 switches context to backend scaffolding; independent of EOS but assumed sequential for this Prompt Book.

---

## P1.5 — Scaffold the ASP.NET Core Backend Solution

**Prerequisites / State Check:** `P1.4` complete. .NET 8 SDK installed locally (`dotnet --version` ≥ 8.0).

**Objective:** Create the `Backend/` .NET solution with three projects — `MonolithV.Api` (REST endpoints), `MonolithV.Data` (data access layer, empty until P1.7/P1.8), `MonolithV.Tests` (xUnit) — with a working health-check endpoint proving the API runs.

**Expected Output:** `dotnet run --project Backend/MonolithV.Api` starts a Kestrel server responding `200 OK` on `GET /health`.

**Files to Modify:**
- `Backend/MonolithV.Backend.sln` (new)
- `Backend/MonolithV.Api/*` (new — Program.cs, MonolithV.Api.csproj, appsettings.json, appsettings.Development.json.example)
- `Backend/MonolithV.Data/MonolithV.Data.csproj` (new, empty class library for now)
- `Backend/MonolithV.Tests/MonolithV.Tests.csproj` (new, xUnit project referencing Api/Data)

**Implementation Steps:**
1. `cd Backend`, `dotnet new sln -n MonolithV.Backend`.
2. `dotnet new webapi -n MonolithV.Api -o MonolithV.Api --use-controllers` (minimal API or controllers — controllers recommended for a clearer REST surface in a report/diagram).
3. `dotnet new classlib -n MonolithV.Data -o MonolithV.Data`.
4. `dotnet new xunit -n MonolithV.Tests -o MonolithV.Tests`.
5. `dotnet sln add MonolithV.Api MonolithV.Data MonolithV.Tests` (add all three csproj paths).
6. `dotnet add MonolithV.Api reference MonolithV.Data`, `dotnet add MonolithV.Tests reference MonolithV.Api MonolithV.Data`.
7. In `MonolithV.Api`, add a `HealthController` with a single `GET /health` action returning `Ok(new { status = "healthy", timestampUtc = DateTime.UtcNow })`.
8. Create `appsettings.Development.json.example` documenting expected keys (`ConnectionStrings:Oracle`, `Redis:ConnectionString`) with placeholder values; add the real `appsettings.Development.json` to `.gitignore`.
9. `dotnet build` the whole solution, confirm zero errors.
10. `dotnet run --project MonolithV.Api`, curl/browse `http://localhost:5000/health` (or whatever port Kestrel picks), confirm the JSON response.
11. Commit.

**Validation Checklist:**
- [ ] `dotnet build` succeeds for all 3 projects
- [ ] `GET /health` returns 200 with the expected JSON shape
- [ ] `appsettings.Development.json` (real, with any local secrets) is gitignored; `.example` version is committed

**Testing Checklist:**
- [ ] `dotnet test` runs (even with zero real tests yet, `MonolithV.Tests` project must at least build and report "0 tests ran" cleanly)

**Git Commit Message:**
```
[P1.5] Scaffold ASP.NET Core backend solution with health endpoint
```

**Documentation Updates:** Start `Docs/Architecture/` component list — add "Backend API (ASP.NET Core, C#)" as a component with one line: "Owns REST endpoints for auth, profile, season, and atomic share/gate transactions."

**Common Mistakes:**
- Using `dotnet new web` (minimal, no controllers) then fighting the routing style later when GAS-adjacent server code expects a clear controller-per-resource layout — decide controllers vs minimal APIs now, don't mix styles later.
- Forgetting to add project references between `Api`/`Data`/`Tests`, causing confusing "type not found" errors only surfaced in P1.7+.

**Next Prompt Dependency:** P1.6 provisions the actual Oracle Cloud resources this backend will eventually connect to.

---

## P1.6 — Provision Oracle Cloud Free Tier: VM + Autonomous Database

**Prerequisites / State Check:** `P1.5` complete. You have (or will create) an Oracle Cloud account (Free Tier).

**Objective:** Stand up the two Oracle Cloud Free Tier resources this whole project depends on for its real deployment story: one Always-Free ARM Ampere Compute VM (future home of the Linux dedicated server + backend container) and one Always-Free Oracle Autonomous Database instance. This prompt is infrastructure-only — nothing runs on them yet beyond a connectivity check.

**Expected Output:** A running Oracle Cloud VM reachable via SSH, and an Autonomous Database instance whose wallet/connection details you can successfully test-connect to from your local machine using SQL*Plus or SQLcl.

**Files to Modify:**
- `Infra/cloud/provision-vm.sh` (new — documents/automates the OCI CLI steps taken)
- `Infra/oracle/README.md` (new — connection instructions, wallet handling)
- `.gitignore` (add `Infra/oracle/wallet/` — the DB wallet must never be committed)

**Implementation Steps:**
1. Sign up for Oracle Cloud Free Tier if you haven't (requires a card for identity verification, but Always-Free resources incur no charge).
2. In OCI Console, create a Compute instance: shape `VM.Standard.A1.Flex` (Ampere, Always-Free eligible), Ubuntu 22.04 LTS image, at least 2 OCPUs / 12GB RAM (within the Always-Free allowance), attach your SSH public key.
3. Open ingress rules on its security list/NSG for: `22` (SSH), and the game's UDP port range you'll use for the dedicated server (reserve e.g. `7777/udp` now, document it — actual server deploy is Phase 5).
4. In OCI Console, create an Autonomous Database (choose "Autonomous Transaction Processing", Always-Free eligible shape), set a strong ADMIN password (store it in a password manager, not in the repo).
5. Download the DB's connection wallet zip; extract it to `Infra/oracle/wallet/` locally (gitignored).
6. Install SQLcl or SQL*Plus locally, test connection: `sql admin@monolithv_high` (or whatever your TNS alias is from `tnsnames.ora` in the wallet), confirm login succeeds.
7. Write `Infra/oracle/README.md` documenting: how to obtain your own wallet (for a teammate or a future you), the TNS alias name, and which service level (`_high`/`_medium`/`_low`) the app will use (use `_medium` as the default balance of throughput/concurrency for a small vertical slice).
8. Write `Infra/cloud/provision-vm.sh` as a documented reference script (OCI CLI commands you ran, parameterized) — even if you did the actual provisioning via Console, this script makes the setup reproducible/auditable for your report.
9. Commit (wallet stays local/gitignored; only the README and script are committed).

**Validation Checklist:**
- [ ] VM is running and reachable via `ssh ubuntu@<vm-public-ip>`
- [ ] Autonomous DB status is "Available" in OCI Console
- [ ] Local SQLcl/SQL*Plus connects successfully using the downloaded wallet
- [ ] Wallet folder is confirmed gitignored (`git status` shows it untracked/ignored)

**Testing Checklist:**
- [ ] `SELECT 1 FROM DUAL;` returns a row via SQLcl
- [ ] `ssh` connection to the VM succeeds and `uname -a` confirms Ubuntu/ARM

**Git Commit Message:**
```
[P1.6] Provision Oracle Cloud Free Tier VM and Autonomous Database
```

**Documentation Updates:** `Docs/Architecture/` — add a one-paragraph "Deployment Target" note: Oracle Cloud Free Tier, ARM VM for compute, Autonomous DB for persistence. This feeds the architecture diagram in P1.14.

**Common Mistakes:**
- Losing the wallet zip or ADMIN password with no backup — store both in a password manager immediately, regenerating a wallet is possible but disruptive mid-project.
- Opening `22/tcp` (or any port) to `0.0.0.0/0` more broadly than necessary — restrict SSH ingress to your current IP where possible, document that the game UDP port is intentionally public.
- Picking a non-Always-Free shape by mistake and incurring charges — double check the "Always Free eligible" badge on both resources before creating.

**Next Prompt Dependency:** P1.7 designs the schema that will be deployed onto this database.

---

## P1.7 — Design & Create Oracle Database Schema v1

**Prerequisites / State Check:** `P1.6` complete, SQLcl/SQL*Plus can connect to the Autonomous DB.

**Objective:** Create the Month-1 schema covering exactly what the vertical slice needs: player accounts/profiles, season state, role assignment, role-item share events (the Golden Apple mechanic), and checkpoint progress — no more, no speculative tables.

**Expected Output:** Five tables live in the Autonomous DB, created via a versioned migration script, with a matching ER diagram draft.

**Files to Modify:**
- `Infra/oracle/schema/V1__init_schema.sql` (new)
- `Docs/Architecture/ER_Diagram.md` (new — text/mermaid description, refined visually in P1.14)

**Implementation Steps:**
1. Design the schema:
   - `PLAYERS` (`PLAYER_ID` PK, `EOS_ACCOUNT_ID` unique, `DISPLAY_NAME`, `CREATED_AT`)
   - `SEASONS` (`SEASON_ID` PK, `SEASON_NUMBER` unique, `STARTED_AT`, `ENDS_AT`, `IS_ACTIVE`)
   - `PLAYER_SEASON_ROLES` (`PLAYER_ID` FK, `SEASON_ID` FK, `ROLE` CHECK IN ('MALE','FEMALE'), PK on (`PLAYER_ID`,`SEASON_ID`)) — enforces the "role chosen per season" rule at the DB level
   - `SHARE_EVENTS` (`SHARE_EVENT_ID` PK, `SEASON_ID` FK, `GIVER_PLAYER_ID` FK, `RECEIVER_PLAYER_ID` FK, `ITEM_TYPE` CHECK IN ('GOLDEN_APPLE','COUNTERPART_ITEM'), `SHARED_AT`) — the record proving a share occurred, checked by the gate mechanic
   - `CHECKPOINT_PROGRESS` (`PLAYER_ID` FK, `SEASON_ID` FK, `CHECKPOINT_INDEX`, `REACHED_AT`, PK on (`PLAYER_ID`,`SEASON_ID`,`CHECKPOINT_INDEX`))
2. Write the full `CREATE TABLE` DDL with explicit constraints (NOT NULL, FKs, CHECK constraints for the role/item enums, since Oracle doesn't have a native enum type) in `V1__init_schema.sql`.
3. Add indexes on foreign keys and on `SHARE_EVENTS(RECEIVER_PLAYER_ID, SEASON_ID)` (the exact lookup the gate-check query will use in P2.6).
4. Run the script against the Autonomous DB via SQLcl: `sql admin@monolithv_medium @V1__init_schema.sql`.
5. Verify all 5 tables exist: `SELECT table_name FROM user_tables;`.
6. Write `Docs/Architecture/ER_Diagram.md` as a Mermaid `erDiagram` block describing these 5 tables and their relationships (renders directly on GitHub).
7. Commit the SQL script and ER doc.

**Validation Checklist:**
- [ ] All 5 tables present in `user_tables`
- [ ] FK constraints verified via `user_constraints`
- [ ] CHECK constraints reject an invalid `ROLE` or `ITEM_TYPE` value when tested manually
- [ ] Mermaid ER diagram renders correctly on GitHub (push and view in browser)

**Testing Checklist:**
- [ ] Manually insert one test row per table (a fake player, season, role, share event, checkpoint) and confirm FK integrity holds (an invalid FK insert fails as expected)
- [ ] Delete the test rows afterward so the schema starts clean for Phase 2 seeding

**Git Commit Message:**
```
[P1.7] Add Oracle schema v1: players, seasons, roles, share events, checkpoints
```

**Documentation Updates:** `Docs/Architecture/ER_Diagram.md` created (this prompt IS the documentation update for this piece).

**Common Mistakes:**
- Adding speculative columns/tables now (inventory, economy, leaderboards) — those are Phase 2+ roadmap; keep v1 exactly matched to the vertical slice's actual needs.
- Forgetting the CHECK constraint on `ROLE`/`ITEM_TYPE` — without it, the "symmetric role-locked item" design rule isn't actually enforced anywhere and bad data becomes possible.

**Next Prompt Dependency:** P1.8 builds the async C# data access layer against this exact schema.

---

## P1.8 — Wire the Async Oracle Data Access Layer

**Prerequisites / State Check:** `P1.7` complete, schema live. `P1.5`'s `MonolithV.Data` project exists (currently empty).

**Objective:** Prove the "no database query ever blocks the networking thread" requirement at the smallest possible scale: an async repository method that reads from `PLAYERS` via `Oracle.ManagedDataAccess.Core`, called from a new `GET /players/{eosAccountId}` endpoint, entirely `async`/`await` with zero `.Result` or `.Wait()` blocking calls anywhere in the path.

**Expected Output:** `GET /players/{eosAccountId}` on the running API returns the player row (or 404) by querying the real Oracle Autonomous DB asynchronously.

**Files to Modify:**
- `Backend/MonolithV.Data/MonolithV.Data.csproj` (add `Oracle.ManagedDataAccess.Core` NuGet package)
- `Backend/MonolithV.Data/OracleConnectionFactory.cs` (new)
- `Backend/MonolithV.Data/PlayerRepository.cs` (new)
- `Backend/MonolithV.Api/Controllers/PlayersController.cs` (new)
- `Backend/MonolithV.Api/appsettings.Development.json.example` (add `ConnectionStrings:Oracle` key)

**Implementation Steps:**
1. `dotnet add MonolithV.Data package Oracle.ManagedDataAccess.Core`.
2. Write `OracleConnectionFactory` with a single method `Task<OracleConnection> CreateOpenConnectionAsync()` that reads the connection string from configuration (injected via `IConfiguration`) and calls `await connection.OpenAsync()`.
3. Write `PlayerRepository` with `Task<PlayerDto?> GetByEosAccountIdAsync(string eosAccountId)`: opens a connection via the factory, builds an `OracleCommand` with a parameterized query (`:eosAccountId` bind variable — never string-concatenate SQL), calls `await command.ExecuteReaderAsync()`, maps the first row if present.
4. Register `PlayerRepository` and the connection factory in `Program.cs` via DI (`builder.Services.AddScoped<...>()`).
5. Add `PlayersController` with `[HttpGet("{eosAccountId}")]` calling the repository async method, returning `Ok(dto)` or `NotFound()`.
6. Set the real (local, gitignored) `ConnectionStrings:Oracle` in `appsettings.Development.json` using the wallet's TNS descriptor or the `tnsnames.ora` alias + `TNS_ADMIN` pointing at the wallet folder path.
7. Insert one test player row via SQLcl, then `dotnet run` the API and `curl` the endpoint, confirm the row comes back as JSON.
8. Confirm (by code review, not just testing) that no method in the call chain uses `.Result`, `.Wait()`, or `Task.Run` to fake-async a blocking call — this is the actual requirement being proven, not just "it works."
9. Commit (excluding real `appsettings.Development.json`).

**Validation Checklist:**
- [ ] `GET /players/<seeded-eos-id>` returns 200 with correct data
- [ ] `GET /players/does-not-exist` returns 404, not a 500
- [ ] Full call chain (`Controller → Repository → OracleCommand`) is `async`/`await` throughout with zero blocking calls
- [ ] SQL uses bind variables, not string concatenation (SQL-injection check)

**Testing Checklist:**
- [ ] Add one xUnit test in `MonolithV.Tests` for `PlayerRepository` using a real (test) DB connection or a documented in-memory fake — at minimum, a test asserting the repository method signature returns `Task<T>` and is awaited correctly in the controller

**Git Commit Message:**
```
[P1.8] Add async Oracle data access layer with non-blocking player lookup
```

**Documentation Updates:** `Docs/Architecture/` — note "Data access pattern: async ODP.NET Core throughout, DI-scoped connections, parameterized queries only" — this becomes a cited design decision in the SRS.

**Common Mistakes:**
- Using `Oracle.ManagedDataAccess` (the .NET Framework package) instead of `Oracle.ManagedDataAccess.Core` — the `.Core` package is required for cross-platform/.NET 8.
- Calling `.GetAwaiter().GetResult()` anywhere "just to make it compile" — this defeats the entire point of this prompt; if something doesn't compile async, fix the signature, don't force sync.
- String-concatenating the `eosAccountId` into SQL — always use bind parameters.

**Next Prompt Dependency:** P1.9 adds Redis caching in front of this same repository pattern.

---

## P1.9 — Add Redis Caching Layer (Local Dev via Docker)

**Prerequisites / State Check:** `P1.8` complete. Docker Desktop (or Docker Engine on your Linux VM) installed locally.

**Objective:** Stand up Redis for local development via `docker-compose`, and cache the `PlayerRepository.GetByEosAccountIdAsync` result behind a short TTL, proving the cache-aside pattern end-to-end without yet needing it at any real scale.

**Expected Output:** `docker-compose up` starts a local Redis container; the `GET /players/{eosAccountId}` endpoint serves the second identical request from Redis (observably faster / confirmed via a log line), not from Oracle.

**Files to Modify:**
- `Infra/docker-compose.yml` (new)
- `Backend/MonolithV.Data/MonolithV.Data.csproj` (add `StackExchange.Redis`)
- `Backend/MonolithV.Data/PlayerCache.cs` (new)
- `Backend/MonolithV.Data/PlayerRepository.cs` (wrap with cache-aside logic, or add a decorator class `CachedPlayerRepository`)
- `Backend/MonolithV.Api/Program.cs` (register Redis connection multiplexer + swap DI registration to the cached decorator)

**Implementation Steps:**
1. Write `Infra/docker-compose.yml` with a single `redis:7-alpine` service, port `6379:6379`, a named volume for persistence (optional at this scale, but demonstrates production-mindedness).
2. `docker compose -f Infra/docker-compose.yml up -d`, confirm `redis-cli ping` returns `PONG`.
3. `dotnet add MonolithV.Data package StackExchange.Redis`.
4. Register `IConnectionMultiplexer` as a singleton in `Program.cs`, reading the Redis connection string from config (`Redis:ConnectionString`, default `localhost:6379`).
5. Implement `CachedPlayerRepository : IPlayerRepository` (extract an `IPlayerRepository` interface from the existing class first) that: checks Redis for key `player:{eosAccountId}` → returns deserialized hit; on miss, calls the real Oracle-backed repository, serializes the result to Redis with a TTL (e.g. 60 seconds — this is a session/profile cache, not a source of truth), then returns it.
6. Swap the DI registration in `Program.cs` so the controller receives `CachedPlayerRepository` (decorator pattern), with the real repository injected into it.
7. Add a temporary `UE_LOG`-style log (`ILogger` in this case) distinguishing "cache hit" vs "cache miss" so you can observe the behavior in the console during testing.
8. Test: call the endpoint twice for the same player, confirm log shows miss-then-hit, and that killing the Oracle connection (or the DB itself briefly) doesn't break the second (cached) call.
9. Commit.

**Validation Checklist:**
- [ ] First request logs "cache miss", hits Oracle
- [ ] Second identical request (within TTL) logs "cache hit", does not hit Oracle
- [ ] After TTL expiry, request falls back to Oracle again correctly
- [ ] `docker-compose.yml` is committed; no Redis data/volumes are committed

**Testing Checklist:**
- [ ] Manual test as described above (hit/miss/expiry) — document actual timings observed in `Docs/Testing/TestPlan.md` (create this file now with a "Phase 1" section)

**Git Commit Message:**
```
[P1.9] Add Redis cache-aside layer for player profile lookups
```

**Documentation Updates:** `Docs/Architecture/` — note the cache-aside pattern and TTL choice; `Docs/Testing/TestPlan.md` created with first entry.

**Common Mistakes:**
- Treating Redis as a source of truth (writing data ONLY to Redis) — it must always be a cache in front of Oracle, never a replacement; if Redis is flushed, correctness must not depend on it.
- Forgetting a TTL entirely — an unbounded cache silently serves stale data forever once profile-mutating endpoints exist in Phase 2/3.

**Next Prompt Dependency:** P1.10 wraps both the Unreal project and the backend solution in CI.

---

## P1.10 — Set Up GitHub Actions CI (Build-Check Only)

**Prerequisites / State Check:** `P1.9` complete. Repo is on GitHub with `main`/`develop`.

**Objective:** A CI workflow that runs on every push/PR, verifying (a) the backend solution builds and its (currently trivial) test suite passes, and (b) documents the Unreal compile-check step (full UE5 CI runners are heavy/self-hosted — for Month 1, document the manual compile-check requirement in the PR template instead of a cloud UE5 build, and automate what's realistically automatable: the backend).

**Expected Output:** A green check on GitHub for every push, running `dotnet build` + `dotnet test` on the `Backend/` solution.

**Files to Modify:**
- `.github/workflows/ci.yml` (new)
- `.github/PULL_REQUEST_TEMPLATE.md` (new — includes an "I compiled the Unreal project locally" checkbox)

**Implementation Steps:**
1. Write `.github/workflows/ci.yml` triggered on `push` and `pull_request` to `main`/`develop`, using `actions/setup-dotnet@v4` (target .NET 8), running `dotnet build Backend/MonolithV.Backend.sln` then `dotnet test Backend/MonolithV.Backend.sln`.
2. Write `.github/PULL_REQUEST_TEMPLATE.md` with a checklist including: "[ ] Unreal project compiles locally with zero errors", "[ ] Relevant `PROJECT_STATE.md` fields updated", "[ ] Commit message includes Prompt ID".
3. Push to a test `feature/ci-check` branch, open a PR into `develop`, confirm the Action runs and reports status on the PR.
4. Once passing, go to repo Settings → Branches → add a protection rule on `main` requiring this check before merge.
5. Merge, commit final workflow to `develop`, then `main` (in line with GitFlow — this is effectively the end of the Phase 1 "release").

**Validation Checklist:**
- [ ] CI runs automatically on push/PR
- [ ] `dotnet build` and `dotnet test` both execute and report correctly
- [ ] Branch protection on `main` requires the check

**Testing Checklist:**
- [ ] Intentionally break the backend build on a throwaway branch, confirm CI fails red; fix it, confirm it goes green

**Git Commit Message:**
```
[P1.10] Add GitHub Actions CI for backend build and test
```

**Documentation Updates:** `Docs/SDLC.md` — note "CI: automated backend build/test; Unreal compile verified manually per PR checklist (documented limitation, revisit if self-hosted runner becomes available)".

**Common Mistakes:**
- Trying to set up a full cloud-hosted Unreal Engine build in CI within Month 1 — this is a real, multi-day DevOps project on its own (self-hosted runners, engine licensing, huge cache sizes); explicitly scope it out and document why, rather than silently skipping it.
- Forgetting `pull_request` trigger (only wiring `push`) — PRs won't show the check inline if only `push` is configured.

**Next Prompt Dependency:** P1.11 provisions your local dev VM; independent of CI but grouped here to close out infra before documentation prompts.

---

## P1.11 — Provision a Local Linux VM for Day-to-Day Dev

**Prerequisites / State Check:** `P1.10` complete. A hypervisor available locally (VirtualBox, VMware, WSL2, or Hyper-V).

**Objective:** Stand up a local Ubuntu Server 22.04 VM that mirrors the Oracle Cloud VM's OS/architecture as closely as practical, used for all day-to-day dedicated-server and backend testing so the Phase 5 cloud deploy is a repeat of a known-working process, not a first attempt.

**Expected Output:** A running local VM reachable via SSH from your host machine, with Docker installed (for the backend+Redis containers) and the .NET 8 runtime installed (for eventually running the backend natively if preferred over Docker there).

**Files to Modify:**
- `Infra/cloud/local-vm-setup.md` (new — step-by-step notes, since this is a manual one-time setup)

**Implementation Steps:**
1. Create an Ubuntu Server 22.04 VM (2+ vCPU, 8GB+ RAM, bridged or NAT-with-port-forward networking so it's reachable from your host on a stable local IP).
2. `apt update && apt upgrade`, then install Docker Engine + Docker Compose plugin, and .NET 8 SDK/runtime (via Microsoft's official apt feed for Ubuntu 22.04).
3. Open the same ports you reserved on the cloud VM (SSH `22`, game UDP port `7777`) on the local VM's firewall (`ufw`) for consistency.
4. Copy `Infra/docker-compose.yml` onto the VM, run `docker compose up -d`, confirm Redis reachable from the VM itself (`redis-cli -h localhost ping`).
5. Document the exact steps taken (versions, commands) in `Infra/cloud/local-vm-setup.md` so this is reproducible and so the same doc can be diffed against the cloud provisioning steps in Phase 5 to catch environment drift early.
6. Commit the documentation (the VM itself obviously isn't committed).

**Validation Checklist:**
- [ ] VM reachable via SSH from host machine
- [ ] Docker + Compose installed and functional
- [ ] .NET 8 SDK installed and `dotnet --version` confirms it
- [ ] Redis container running on the VM

**Testing Checklist:**
- [ ] From host machine, `curl` a manually-started `dotnet run` instance of `MonolithV.Api` running on the VM, confirm reachability over the local network

**Git Commit Message:**
```
[P1.11] Document local Linux VM setup for day-to-day dev
```

**Documentation Updates:** `Docs/InstallationGuide.md` — add "Local Dev VM" section pointing at `Infra/cloud/local-vm-setup.md`.

**Common Mistakes:**
- Using a different Ubuntu version/architecture (e.g. x86 locally vs ARM on Oracle Cloud) without documenting the difference — fine to do for cost/availability reasons, but write down the discrepancy so Phase 5 deployment scripts account for it (e.g. any ARM-specific Docker image tags).

**Next Prompt Dependency:** P1.12 begins the documentation track, referencing all infra decisions made in P1.1–P1.11.

---

## P1.12 — Draft the Software Requirements Specification (SRS)

**Prerequisites / State Check:** P1.1–P1.11 complete — SRS must describe what's actually been decided/built, not aspirational content.

**Objective:** Produce `Docs/SRS.md` covering: introduction/purpose, scope (explicitly stating the vertical-slice Month-1 boundary vs. Future Roadmap), functional requirements (per the 4 pillars + gameplay loop), non-functional requirements (30Hz tick, async-only DB access, etc.), and system constraints (solo dev, 1 month, Oracle Cloud Free Tier limits).

**Expected Output:** A complete, internally consistent SRS document that could stand alone in front of an evaluator without needing this conversation as context.

**Files to Modify:**
- `Docs/SRS.md` (new)

**Implementation Steps:**
1. Write section 1 (Introduction): purpose, intended audience (evaluators/collaborators), definitions (Monolith, altitude band, guardian, share-gate, season).
2. Write section 2 (Overall Description): product perspective (standalone multiplayer game + supporting backend), product functions (bullet summary of the core loop), user classes (single class: "Player"), operating environment (Windows client, Linux dedicated server).
3. Write section 3 (Functional Requirements), one subsection per pillar, each with numbered requirements (`FR-1.1`, `FR-1.2`, ...):
   - Server-Authoritative Sync (30Hz tick, client prediction, server validates all movement/combat)
   - Procedural Memory Optimization (altitude-indexed chunk streaming, load/unload behavior)
   - Atomic Shared-State Data Engine (share-event validation, race-condition handling on simultaneous share attempts)
   - Async Database Layer (no blocking calls, Oracle + Redis cache-aside)
   - Gameplay requirements (role selection, share mechanic, checkpoints, guardian combat, jetpack traversal)
4. Write section 4 (Non-Functional Requirements): performance targets (tick rate, target concurrent players 2-4), security (parameterized queries, server-authoritative validation as the anti-cheat baseline), maintainability (module boundaries matching the folder structure).
5. Write section 5 (Constraints & Assumptions): solo developer, 1-month timeline, Oracle Cloud Free Tier resource limits, explicitly listing what is OUT of scope for Month 1 (link to README's Future Roadmap).
6. Cross-check every functional requirement against something that actually exists or is concretely planned in Phase 2/3 prompts — delete any requirement that doesn't map to a real planned prompt.
7. Commit.

**Validation Checklist:**
- [ ] Every FR has a unique ID and traces to a specific Phase 2/3/4 prompt (add a "traces to" note per FR)
- [ ] Scope section explicitly excludes Future Roadmap items
- [ ] Document reads standalone (no "as discussed" references)

**Testing Checklist:**
- [ ] Have a friend/classmate read the SRS cold and confirm they understand the project without additional explanation (informal readability check)

**Git Commit Message:**
```
[P1.12] Draft Software Requirements Specification
```

**Documentation Updates:** This prompt IS a documentation deliverable.

**Common Mistakes:**
- Writing the SRS to match the original AAA-scale abstract instead of the actual scoped vertical slice — this creates a document that contradicts what you actually submit; the SRS must describe the real Month-1 system, with the bigger vision clearly labeled as future work.

**Next Prompt Dependency:** P1.13 formalizes the SDLC document alongside this SRS.

---

## P1.13 — Formalize the SDLC Document

**Prerequisites / State Check:** `P1.12` complete.

**Objective:** Produce `Docs/SDLC.md`, formalizing the README's SDLC roadmap table into a full academic document: methodology justification (why Incremental/Iterative fits a solo 1-month project better than Waterfall or full Scrum), phase-by-phase deliverables, and roles (trivial for solo, but state it explicitly for completeness).

**Files to Modify:**
- `Docs/SDLC.md` (new)

**Implementation Steps:**
1. Write a methodology section justifying Incremental/Iterative: fixed phase gates (good for grading/checkpoints) without a costly full requirements-freeze (impossible solo in 1 month) or full Scrum ceremony overhead (unnecessary for a team of one).
2. Reproduce and expand the README's SDLC table with explicit entry/exit criteria per phase (e.g. Phase 2 exit criteria = M2 milestone from README).
3. Add a RACI-style note (even if trivial: "Developer: sole owner of all roles — requirements, design, implementation, testing, deployment").
4. Cross-reference `Docs/RiskAnalysis.md` (create as a stub now, filled in properly in Phase 4) for known risks per phase.
5. Commit.

**Validation Checklist:**
- [ ] Methodology justification present and specific to this project's actual constraints (not generic textbook boilerplate)
- [ ] Phase entry/exit criteria match README milestones exactly

**Testing Checklist:**
- [ ] Cross-check every phase's exit criteria against the corresponding milestone checkbox in `README.md` — must match verbatim in meaning

**Git Commit Message:**
```
[P1.13] Formalize SDLC methodology document
```

**Documentation Updates:** `Docs/RiskAnalysis.md` stub created.

**Common Mistakes:**
- Picking "Agile/Scrum" label without justifying it for a solo dev — a grader will ask "who's your Scrum Master then?"; be honest that this is an adapted Incremental model, not textbook Scrum.

**Next Prompt Dependency:** P1.14 produces the visual architecture diagrams referenced by both SRS and SDLC.

---

## P1.14 — Draft Initial Architecture Diagrams

**Prerequisites / State Check:** P1.1–P1.13 complete — every component referenced in these diagrams must already exist in the repo (even if just scaffolded) or be a Phase 2/3 prompt ID you can cite.

**Objective:** Produce the first real versions of the Class Diagram, Activity Diagram, Sequence Diagram, DFD, and Flowchart (ER Diagram already exists from P1.7), using Mermaid syntax so they render natively on GitHub and are diffable in version control (avoid opaque binary diagram files for these first drafts).

**Files to Modify:**
- `Docs/Architecture/Class_Diagram.md`
- `Docs/Architecture/Activity_Diagram.md`
- `Docs/Architecture/Sequence_Diagram.md`
- `Docs/Architecture/DFD.md`
- `Docs/Architecture/Flowchart.md`

**Implementation Steps:**
1. **Class Diagram**: `classDiagram` covering `AMonolithVCharacter`, `AMonolithVGameMode`, `PlayerRepository`/`CachedPlayerRepository`, `PlayerCache` — expand this diagram again after Phase 2/3 add real gameplay classes; this first pass proves the tool/format works and captures what exists today.
2. **Activity Diagram**: `flowchart TD` (Mermaid's activity-style) showing the intended player session: Login (EOS) → Select/confirm season role → Spawn in world → Climb (fight guardians / gain altitude) → Reach share-gated checkpoint → (if paired) Share item → Continue → Reach top or die → Respawn/season end.
3. **Sequence Diagram**: `sequenceDiagram` for the one concrete flow that already partially exists: Client → Dedicated Server (movement replication) and API → Oracle (player lookup) → Redis (cache) — this is deliberately the Phase 1 slice of the sequence, expanded in Phase 2 once the share-gate transaction exists end-to-end.
4. **DFD**: Level-0 (context) diagram showing external entity "Player" and processes "Game Client/Server", "Backend API", "Oracle DB", "Redis Cache", with data flows labeled (login token, player state, share events).
5. **Flowchart**: the share-gate mechanic's decision logic specifically (this is the most "interesting" algorithmic piece to diagram): "Checkpoint reached → Query SHARE_EVENTS for this pair this season → Exists? → Yes: unlock / No: block + prompt".
6. Push and view each `.md` file on GitHub to confirm Mermaid rendering (GitHub renders Mermaid natively in markdown preview).
7. Commit all five files together.

**Validation Checklist:**
- [ ] All 5 diagrams render correctly on GitHub (not just locally in an editor preview)
- [ ] Class diagram only includes classes that exist in the repo right now (no invented future classes)
- [ ] Sequence/Activity/DFD/Flowchart accurately reflect the Phase 1 state, explicitly noting "(expanded in Phase 2/3)" where relevant

**Testing Checklist:**
- [ ] Open each diagram file's raw GitHub URL in a browser and visually confirm correct rendering (no Mermaid syntax errors)

**Git Commit Message:**
```
[P1.14] Draft initial architecture diagrams (class, activity, sequence, DFD, flowchart)
```

**Documentation Updates:** This prompt completes the initial `Docs/Architecture/` set (ER done in P1.7, five more here).

**Common Mistakes:**
- Diagramming the full future vision (infinite tower, matchmaking, seasons at scale) instead of the actual Phase 1/2/3 system — these diagrams should grow incrementally alongside the real code, revisited explicitly in Phase 3 and Phase 5, not written once as a guess up front.

**Next Prompt Dependency:** This is the last prompt of Phase 1. Before moving to `02_Core_Architecture.md`, update `PROJECT_STATE.md`: `Last Completed Prompt ID: P1.14`, `Current Phase: Phase 1 complete → Phase 2`, `Next Prompt To Run: P2.1`. Tag the repo `phase-1-complete` and merge `develop` → `main` per GitFlow (this is the "release" point for Phase 1). This is also a natural Twitter/X milestone post moment: repo is public, foundation is real and running (health endpoint, DB, cache, CI, EOS skeleton) — a good "here's the skeleton coming to life" post.
