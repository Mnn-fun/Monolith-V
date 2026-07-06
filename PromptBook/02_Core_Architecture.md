# Phase 2 — Core Architecture

Goal of this phase: the four abstract pillars become real, running, networked systems — a dedicated Linux server authoritatively replicating movement at 30Hz with client prediction, GAS wired for combat, altitude-indexed chunk streaming proven, and a concurrency-safe backend transaction layer for the share/checkpoint mechanics. No finished gameplay content yet (that's Phase 3) — this phase proves the plumbing works under real network conditions.

---

## P2.1 — Build & Package the Dedicated Server Target

**Prerequisites / State Check:** `PROJECT_STATE.md` → `Last Completed Prompt ID: P1.14`. `Game/Source/MonolithVServer` target exists from P1.2.

**Objective:** Produce a real, runnable headless Linux dedicated server binary from the `MonolithVServer` target, confirm it runs on your local Linux VM (from P1.11) and accepts a game client connection over the network.

**Expected Output:** A packaged Linux server build launchable via `./MonolithVServer.sh MonolithV_Map -log`, reachable from your Windows host's Unreal client via direct IP connect.

**Files to Modify:**
- `Game/Config/DefaultEngine.ini` (confirm dedicated server net settings)
- `Infra/cloud/local-vm-setup.md` (append server deployment notes)

**Implementation Steps:**
1. In UE5 Editor, confirm Linux cross-compilation toolchain is installed (Platform SDK for Linux, via Epic's clang cross-toolchain installer for your engine version).
2. Package via `Tools > Package Project` is GUI-only for client builds; for the server target use `RunUAT.bat BuildCookRun -project="Game/MonolithV.uproject" -platform=Linux -clientconfig=Development -serverconfig=Development -server -noclient -cook -build -stage -pak -archive -archivedirectory="Builds/LinuxServer"`.
3. Copy the resulting `Builds/LinuxServer/LinuxServer/` folder to the local VM (`scp -r`).
4. On the VM, `chmod +x MonolithVServer.sh`, run `./MonolithVServer.sh MonolithV_Map -log -port=7777`.
5. On your Windows host, package or run a standalone client (`-game` or a Development client build) and connect via console: `open <vm-local-ip>:7777`.
6. Confirm in the server's log that a client connection is accepted and a pawn is spawned; confirm on the client that it sees itself in the world.
7. Document the exact `RunUAT` command and VM launch command in `Infra/cloud/local-vm-setup.md` so it's copy-pasteable for every future test, and reusable directly in Phase 5's cloud deploy script.
8. Commit documentation (binaries are not committed — add `Builds/` to `.gitignore`).

**Validation Checklist:**
- [ ] Server binary runs headless on Linux VM with no missing shared library errors
- [ ] Client successfully connects via direct IP
- [ ] Server log shows the connection and pawn spawn

**Testing Checklist:**
- [ ] Connect two separate client instances to the same server, confirm both are logged as connected

**Git Commit Message:**
```
[P2.1] Build and deploy headless Linux dedicated server binary
```

**Documentation Updates:** `Infra/cloud/local-vm-setup.md` — add "Running the Dedicated Server" section with exact commands.

**Common Mistakes:**
- Forgetting `-noclient` on a server-only package, which bloats the build with client-only assets/code.
- Not opening the `7777/udp` port on the VM's firewall (`ufw allow 7777/udp`) — connections silently time out.
- Testing only with the Editor's "Play as Listen Server" and assuming that proves the dedicated server target works — they are different binaries; always validate the actual packaged `MonolithVServer` build.

**Next Prompt Dependency:** P2.2 builds the real replicated movement this connection test was standing in for.

---

## P2.2 — Server-Authoritative Character Movement

**Prerequisites / State Check:** `P2.1` complete — you can connect a client to a packaged dedicated server.

**Objective:** Implement real replicated character movement using `UCharacterMovementComponent`'s built-in networked movement (server-authoritative by default), extended with the project's own `AMonolithVCharacter`, proving the server is the sole authority on position — a client cannot move without the server validating and replicating the result back.

**Expected Output:** Two clients connected to one dedicated server can walk around and see each other's movement, with all position state authoritatively owned by the server (verified by attempting to "cheat" position client-side and observing the server correct it).

**Files to Modify:**
- `Game/Source/MonolithV/Player/MonolithVCharacter.h/.cpp`
- `Game/Source/MonolithV/Player/MonolithVPlayerController.h/.cpp` (new)
- `Game/Config/DefaultInput.ini` / new `Enhanced Input` assets (movement bindings)

**Implementation Steps:**
1. Create `AMonolithVPlayerController : public APlayerController`, set as the default player controller class in `AMonolithVGameMode`.
2. In `AMonolithVCharacter`, wire Enhanced Input (UE5's default input system) actions for Move/Look, calling `AddMovementInput` — this feeds `UCharacterMovementComponent`'s built-in client-predicted, server-corrected movement pipeline (do not hand-roll a custom movement replication system; `CharacterMovementComponent` already implements exactly the server-authoritative + client-prediction pattern this project needs).
3. Confirm `bUseControllerRotationYaw` / movement component settings (`MaxWalkSpeed`, `GravityScale`) are set deliberately, not left at engine defaults, and documented as intentional values (these become tuning knobs later for jetpack traversal in Phase 3).
4. Test the "cheat check": in a client-side debug console, attempt `SetActorLocation` directly on the locally controlled pawn outside of normal movement input — confirm the server's next replication update snaps it back (proving the server, not the client, owns truth).
5. Add a `UPROPERTY(ReplicatedUsing=OnRep_Health)` placeholder float `Health` on the character now (used for real in P2.4's GAS AttributeSet, but wire the basic replication callback pattern here first as a simple, well-understood example before GAS adds complexity).
6. Commit.

**Validation Checklist:**
- [ ] Two clients see each other move smoothly
- [ ] Client-side position tampering is corrected by the server within one tick
- [ ] `Health` replicates via `OnRep_Health` correctly when changed server-side (test with a temporary console command that decrements it)

**Testing Checklist:**
- [ ] Artificially add latency (Unreal's `PktLag=100` net emulation console command) and confirm movement still feels reasonable and remains server-corrected

**Git Commit Message:**
```
[P2.2] Implement server-authoritative character movement
```

**Documentation Updates:** `Docs/Architecture/Sequence_Diagram.md` — expand with the movement replication sequence (Client Input → Local Prediction → Server Validation → Correction Broadcast).

**Common Mistakes:**
- Hand-rolling a custom RPC-based movement system instead of using `UCharacterMovementComponent`'s built-in networked movement — this is reinventing a solved, heavily-optimized Unreal subsystem and is a common source of subtle bugs; only replace it if a specific documented reason requires it (this project doesn't).
- Setting `Role`/`RemoteRole` manually — Unreal manages these automatically for `Character`/`Pawn`; manual interference usually breaks replication.

**Next Prompt Dependency:** P2.3 adds explicit client-side prediction/interpolation tuning on top of this baseline for the "30Hz doesn't feel laggy" requirement.

---

## P2.3 — Client-Side Prediction & Interpolation Tuning

**Prerequisites / State Check:** `P2.2` complete.

**Objective:** Explicitly tune and document the prediction/interpolation settings that make a 30Hz server tick feel responsive, since this was a deliberate architectural decision (P1.3) instead of raising tick rate — this prompt is where that decision is proven correct or revisited with data.

**Expected Output:** Documented, tuned values for `NetworkSmoothingMode`, `NetUpdateFrequency`, `MinNetUpdateFrequency` on `AMonolithVCharacter`, with a before/after latency-feel comparison recorded in the test plan.

**Files to Modify:**
- `Game/Source/MonolithV/Player/MonolithVCharacter.cpp` (constructor: `NetUpdateFrequency`, `MinNetUpdateFrequency`, `NetworkSmoothingMode` on the movement component)
- `Docs/Testing/TestPlan.md`

**Implementation Steps:**
1. Set `NetUpdateFrequency = 30.f` (matching server tick) and `MinNetUpdateFrequency = 10.f` on the character.
2. Set `GetCharacterMovement()->NetworkSmoothingMode = ENetworkSmoothingMode::Exponential` (client-side smoothing of the server-corrected position, standard Unreal approach for exactly this "30Hz feels laggy otherwise" problem).
3. Using Unreal's net emulation console commands (`Net PktLag=50`, `PktLagVariance=25`), test movement feel at simulated 50-100ms latency with smoothing on vs. off, and record subjective + objective (position-correction magnitude, logged via a temporary debug draw of server vs. predicted position) notes.
4. Document the specific numbers chosen and why in `Docs/Testing/TestPlan.md` under a new "Networking — Tick Rate & Smoothing" section — this is the evidence backing the P1.3/README architectural decision when questioned at your defense/presentation.
5. Commit.

**Validation Checklist:**
- [ ] Movement feels acceptable (no visible rubber-banding) at 50-100ms simulated latency in manual testing
- [ ] Settings and rationale are documented with actual observed numbers, not just "it feels fine"

**Testing Checklist:**
- [ ] Record a short screen capture comparing `NetworkSmoothingMode::Linear` vs `Exponential` vs `None` at the same simulated latency — keep this clip, it's excellent presentation material

**Git Commit Message:**
```
[P2.3] Tune client-side prediction and interpolation for 30Hz tick feel
```

**Documentation Updates:** `Docs/Testing/TestPlan.md` — networking section added with real measurements.

**Common Mistakes:**
- Tuning by feel alone with no recorded measurements — you will be asked "how do you know 30Hz is enough" in your defense; have the emulated-latency test notes ready.

**Next Prompt Dependency:** P2.4 builds GAS on top of this now-solid movement foundation.

---

## P2.4 — Gameplay Ability System (GAS) Skeleton

**Prerequisites / State Check:** `P2.3` complete. `GameplayAbilities` module already referenced in `MonolithV.Build.cs` since P1.2.

**Objective:** Wire the minimum real GAS setup — `UAbilitySystemComponent`, a `UMonolithVAttributeSet` (Health, MaxHealth to start), and one working, replicated test `UGameplayAbility` (a simple damage-self or heal-self ability) — proving the ability-activation-and-replication pipeline before Phase 3 builds real combat/traversal abilities on it.

**Expected Output:** Pressing a bound test key activates a GAS ability that modifies `Health` via a `GameplayEffect`, replicated correctly to all clients, replacing the placeholder `Health` property from P2.2.

**Files to Modify:**
- `Game/Source/MonolithV/Player/MonolithVCharacter.h/.cpp` (add `UAbilitySystemComponent*`, implement `IAbilitySystemInterface`)
- `Game/Source/MonolithV/Combat/MonolithVAttributeSet.h/.cpp` (new)
- `Game/Source/MonolithV/Combat/GA_TestAbility.h/.cpp` (new)
- `Game/Source/MonolithV/Combat/GE_TestDamage.h/.cpp` (new `UGameplayEffect` subclass or Blueprint-based effect, C++ base)

**Implementation Steps:**
1. Add `UAbilitySystemComponent* AbilitySystemComponent` to `AMonolithVCharacter`, implement `IAbilitySystemInterface::GetAbilitySystemComponent()`, set replication mode `EGameplayEffectReplicationMode::Mixed` (appropriate for a single-owned-character-per-player setup).
2. Create `UMonolithVAttributeSet` with `Health`/`MaxHealth` as `FGameplayAttributeData`, implement `GetLifetimeReplicatedProps` and `OnRep_Health`/`OnRep_MaxHealth`, remove the old plain `Health` UPROPERTY from P2.2 (GAS attributes replace it).
3. Implement `PreAttributeChange`/`PostGameplayEffectExecute` on the attribute set to clamp `Health` between 0 and `MaxHealth`.
4. Create `GE_TestDamage`, a simple instant `GameplayEffect` that subtracts a fixed amount from `Health`.
5. Create `GA_TestAbility : public UGameplayAbility`, granted to the character on `PossessedBy` (server-side), bound to a test input key via Enhanced Input → `AbilityLocalInputPressed`, applying `GE_TestDamage` to self on activation.
6. Initialize the `AttributeSet`'s base values (`Health = MaxHealth = 100`) in `BeginPlay` on the server only, then confirm the `AbilitySystemComponent`'s `InitAbilityActorInfo` is called correctly for both server and client (in `PossessedBy` and `OnRep_PlayerState` respectively — this dual-init is the single most common GAS setup bug).
7. Test in a 2-client session: activate the ability on Client A, confirm Client B sees Client A's health bar (once UI exists in Phase 3) or at minimum sees correct replicated `Health` value via debug log.
8. Commit.

**Validation Checklist:**
- [ ] Ability activates only after server confirms (standard GAS prediction key flow, not a raw client-trusted action)
- [ ] `Health` change replicates correctly to all clients
- [ ] `InitAbilityActorInfo` called in both `PossessedBy` (server) and `OnRep_PlayerState` (client) — verified via debug log placed in each

**Testing Checklist:**
- [ ] Two clients, one activates the ability, both observe the correct final `Health` value via log
- [ ] Attempt to activate the ability from a client with an artificially blocked server RPC (simulate via `Net PktLoss=100` briefly) and confirm the ability does NOT silently apply client-side only

**Git Commit Message:**
```
[P2.4] Add GAS skeleton with replicated attribute set and test ability
```

**Documentation Updates:** `Docs/Architecture/Class_Diagram.md` — add `AbilitySystemComponent`, `MonolithVAttributeSet`, `GA_TestAbility`, `GE_TestDamage`.

**Common Mistakes:**
- Forgetting the client-side `InitAbilityActorInfo` call in `OnRep_PlayerState` — abilities will seem to "work on server but do nothing visually on client" without it.
- Applying damage/effects directly by setting the attribute instead of going through a `GameplayEffect` — bypasses GAS's prediction/replication machinery and reintroduces the exact race-condition risk GAS exists to prevent.

**Next Prompt Dependency:** P2.5 builds the altitude-streaming system independently; grouped next before backend work.

---

## P2.5 — Altitude-Indexed Chunk Streaming Core

**Prerequisites / State Check:** `P2.4` complete (independent system, but sequenced here to finish all client/server gameplay-adjacent systems before backend prompts).

**Objective:** Implement the "Procedural Memory Optimization" pillar for real: a manager that divides the world into altitude bands (chunks), streams a band's level/actors in as a player's Z-height approaches it and purges bands far below/above the player, using Unreal's native level streaming API rather than a hand-rolled asset-loading system.

**Expected Output:** A `AAltitudeStreamingManager` running on the server, tracking each player's current altitude band, calling `UGameplayStatics::LoadStreamLevel` / `UnloadStreamLevel` on `ULevelStreamingDynamic` instances as players cross band thresholds, with a debug HUD/log showing current loaded bands and a memory usage delta proving unloaded bands actually free memory.

**Files to Modify:**
- `Game/Source/MonolithV/World/AltitudeStreamingManager.h/.cpp` (new)
- `Game/Source/MonolithV/World/AltitudeBand.h` (new — a simple struct/data asset: band index, Z-range, associated streaming level name)
- `Game/Content/Maps/` (new sublevels, one per band, greyboxed for now — real art comes in Phase 3)

**Implementation Steps:**
1. Design the band model: 5 bands for the Month-1 vertical slice, each 2000 Unreal units of Z-height, each corresponding to one sublevel (`Band_00`, `Band_01`, ... `Band_04`), matching "every 5 floors = 1 level" from your design (each band here = one "floor tier" of the eventual infinite tower; the manager is written generically so adding `Band_05`+ later is pure data, not new code — this is the concrete proof of the "infinitely scalable" claim).
2. Implement `AAltitudeStreamingManager` as a server-only actor (spawned by the GameMode), holding an array of `FAltitudeBand` data and a `TMap<APlayerState*, int32> CurrentBandPerPlayer`.
3. On a timer (or on each character's `Tick`, throttled to ~1Hz — this doesn't need 30Hz), compute each player's current band from their Z-height, and for each player: ensure their current band ± 1 (load buffer) is streamed in, and trigger unload for any band no player is within ±1 of.
4. Use `ULevelStreamingDynamic::LoadLevelInstance` / call `SetShouldBeLoaded`/`SetShouldBeVisible` on persistent `ULevelStreaming` objects (either approach is acceptable — pick one and document why; `LoadLevelInstance` is simpler to reason about for a first implementation).
5. Add a debug on-screen display (`GEngine->AddOnScreenDebugMessage`) listing currently loaded bands and each player's current band, for visual proof during testing/demo.
6. Test memory impact: use `stat memory` before and after forcing a player through all 5 bands, confirm loaded/unloaded transitions actually change memory usage (not just visually removing geometry while keeping it resident).
7. Commit.

**Validation Checklist:**
- [ ] Bands load/unload correctly as a player crosses Z-thresholds
- [ ] `stat memory` shows a measurable decrease after a band unloads
- [ ] Adding a 6th band requires only a new data entry + sublevel, zero code changes (test this explicitly — add a throwaway 6th band, confirm it works, then remove it if not wanted yet for Month 1 content)

**Testing Checklist:**
- [ ] Two players in different bands simultaneously — confirm the manager keeps both players' relevant bands loaded independently (this is the actual multi-tenant test, not just single-player streaming)
- [ ] Record `stat memory` numbers before/after in `Docs/Testing/TestPlan.md`

**Git Commit Message:**
```
[P2.5] Implement altitude-indexed chunk streaming manager
```

**Documentation Updates:** `Docs/Architecture/` — this is the centerpiece diagram for pillar 2; add a dedicated `Docs/Architecture/ChunkStreaming.md` with a Mermaid diagram of the band-load/unload state machine.

**Common Mistakes:**
- Running the band-check logic on client as well as server — this must be server-authoritative (matching pillar 1) since streaming decisions affect what exists in the authoritative world state; clients should only reflect what the server has streamed, via replication of relevant actors.
- Checking altitude on every `Tick` for every player at full frequency — unnecessary CPU cost; throttle this check (1Hz is plenty for band transitions).
- Confusing "actor visibility culling" with "memory freed" — verify with `stat memory`, not just visual disappearance, or the "optimization" claim in your report is unsubstantiated.

**Next Prompt Dependency:** P2.6 wires real player identity (EOS) so the streaming manager's per-player tracking has a real, persistent player identity to key off going forward.

---

## P2.6 — EOS Login & Session Join Flow

**Prerequisites / State Check:** `P2.5` complete. EOS skeleton from P1.4 confirmed working (`IOnlineSubsystem::Get(TEXT("EOS"))` non-null).

**Objective:** Implement the real login flow (EOS Connect / Auth interface, dev-auth-tool or Epic account login for testing) and session creation/join (EOS Sessions interface) so two players find and join the same dedicated server session through EOS rather than a hardcoded direct-IP connect — direct-IP remains a fallback debug path.

**Expected Output:** A client can log in via EOS, create or find a session hosted by your dedicated server, and join it, with the resulting `EOS_ProductUserId` available to associate with the `PLAYERS.EOS_ACCOUNT_ID` column from P1.7.

**Files to Modify:**
- `Game/Source/MonolithV/Networking/EOSLoginSubsystem.h/.cpp` (new, a `UGameInstanceSubsystem`)
- `Game/Source/MonolithV/Networking/EOSSessionSubsystem.h/.cpp` (new)
- `Game/Source/MonolithV/MonolithVGameMode.cpp` (register session as the dedicated server's advertised session)

**Implementation Steps:**
1. Use the EOS Dev Auth Tool (bundled with the EOS SDK) to create a local test login credential for development (avoids requiring a full Epic account flow for every test iteration).
2. Implement `UEOSLoginSubsystem::Login()` calling `IOnlineIdentity::Login()` against the EOS subsystem with dev-auth credentials, exposing a `FOnLoginComplete` delegate.
3. Implement `UEOSSessionSubsystem::CreateSession()` (server-side, called by the GameMode on server startup) using `IOnlineSession::CreateSession()` with a defined `FOnlineSessionSettings` (max players, public/searchable), and `FindSessions()`/`JoinSession()` (client-side).
4. On successful join, use the resulting `EOS_ProductUserId` (retrieved via `IOnlineIdentity::GetUniquePlayerId`) as the canonical player identity going forward — this is the ID sent to the backend API in P2.9+ and stored as `EOS_ACCOUNT_ID`.
5. Test end-to-end: both clients log in via EOS Dev Auth Tool identities, one finds and joins the other's (or the dedicated server's) advertised session, both end up connected to the same `MonolithVServer` instance.
6. Keep the direct-IP `open <ip>:7777` path available as a documented fallback debug command (useful when EOS backend has transient issues during a live demo).
7. Commit.

**Validation Checklist:**
- [ ] Login succeeds via EOS Dev Auth Tool identity
- [ ] Session search finds the dedicated server's advertised session
- [ ] Join succeeds and results in the same connected-client state as the P2.1 direct-IP test
- [ ] `EOS_ProductUserId` retrieved correctly and logged

**Testing Checklist:**
- [ ] Two separate dev-auth identities, full login → find → join flow, both connected simultaneously
- [ ] Fallback direct-IP connect still works as a documented backup

**Git Commit Message:**
```
[P2.6] Implement EOS login and session join flow
```

**Documentation Updates:** `Docs/UserManual.md` — add "Logging In" section (even if it's a dev-tool flow for now, document what a real Epic account login will look like once out of dev sandbox).

**Common Mistakes:**
- Building session join logic only against Play-in-Editor and never validating against the actual packaged dedicated server — EOS session advertisement behaves differently for a true dedicated server (`bIsDedicated=true` in session settings) vs. a listen server; get this flag right or joins will subtly misbehave.

**Next Prompt Dependency:** P2.7 and P2.8 build the backend transaction endpoints that this real player identity will call into.

---

## P2.7 — Atomic Share-Event Transaction Endpoint

**Prerequisites / State Check:** `P2.6` complete (real player identity available). `P1.7` schema (`SHARE_EVENTS` table) and `P1.8` async data layer pattern established.

**Objective:** Implement the concurrency-safe backend endpoint recording a Golden-Apple/counterpart-item share — this is the abstract's "multi-threaded transaction validation protocol...mitigating race conditions and state duplication" pillar made concrete: two near-simultaneous share attempts for the same giver/receiver/season/item must never produce duplicate rows or a corrupted state.

**Expected Output:** `POST /seasons/{seasonId}/share-events` that is idempotent and race-safe under concurrent identical requests, verified with an actual concurrent-load test, not just sequential manual calls.

**Files to Modify:**
- `Backend/MonolithV.Data/ShareEventRepository.cs` (new)
- `Backend/MonolithV.Api/Controllers/ShareEventsController.cs` (new)
- `Backend/MonolithV.Tests/ShareEventConcurrencyTests.cs` (new)
- `Infra/oracle/schema/V2__share_event_unique_constraint.sql` (new — add a UNIQUE constraint if not already present from P1.7's design)

**Implementation Steps:**
1. Confirm/add a UNIQUE constraint on `SHARE_EVENTS(SEASON_ID, GIVER_PLAYER_ID, RECEIVER_PLAYER_ID, ITEM_TYPE)` — this is the database-level defense-in-depth layer (the "final authority") against duplicate shares, independent of any application logic.
2. Implement an in-process, keyed async lock: a `ConcurrentDictionary<string, SemaphoreSlim>` keyed by `$"{seasonId}:{giverPlayerId}:{receiverPlayerId}:{itemType}"`, so concurrent requests for the *same* pair/item serialize through the semaphore before even reaching the database (reduces DB contention/wasted round-trips under load, and is the concrete "multi-threaded transaction validation" mechanism the abstract describes) — concurrent requests for *different* pairs proceed fully in parallel, unaffected.
3. Inside the semaphore-guarded section: validate the giver/receiver have opposite `PLAYER_SEASON_ROLES.ROLE` values for this season (query `PLAYER_SEASON_ROLES`, reject with `400` if same role or either role missing), then attempt the `INSERT` inside an explicit Oracle transaction.
4. Catch the unique-constraint-violation exception on insert (Oracle error `ORA-00001`) and treat it as a successful no-op (`200 OK` with `{ alreadyShared: true }`) rather than an error — this makes the endpoint truly idempotent even if the in-process semaphore layer were ever bypassed (e.g. a future multi-instance backend deployment where in-memory locks alone wouldn't be enough — document this as exactly why the DB constraint remains the real authority).
5. Write `ShareEventConcurrencyTests`: fire 20 concurrent identical `POST` requests (via `Task.WhenAll` against a test server instance, e.g. `WebApplicationFactory`), assert exactly one row exists in `SHARE_EVENTS` afterward and all 20 responses are successful (`200`, either the real insert or the idempotent no-op).
6. Commit.

**Validation Checklist:**
- [ ] Role-mismatch share attempts rejected with 400
- [ ] 20 concurrent identical requests produce exactly 1 database row
- [ ] All 20 concurrent requests return a success status (no 500s from a lock/constraint race)
- [ ] Unique constraint confirmed present via `user_constraints` query

**Testing Checklist:**
- [ ] `ShareEventConcurrencyTests` passes reliably across at least 5 repeated runs (concurrency bugs are often flaky — run it enough times to trust it)
- [ ] Manual test via two real Unreal clients performing a share simultaneously (as close to simultaneous as manually possible) confirms correct single-row result

**Git Commit Message:**
```
[P2.7] Add concurrency-safe atomic share-event transaction endpoint
```

**Documentation Updates:** `Docs/Architecture/Sequence_Diagram.md` — add the share-event sequence including the semaphore + DB-constraint dual-layer; this is likely your single best diagram for demonstrating pillar 3 at your defense.

**Common Mistakes:**
- Relying on the in-process semaphore alone as "the" concurrency solution — it only works within a single backend process/instance; the DB unique constraint is what makes correctness hold even if you later scale the backend horizontally (documented as a Future Roadmap item), so both layers must exist, not just one.
- Testing concurrency sequentially ("I called it twice in a row and it worked") — this proves nothing about race conditions; the test must fire genuinely concurrent requests (`Task.WhenAll`, not a `for` loop with `await` each iteration).

**Next Prompt Dependency:** P2.8 applies the same rigor to checkpoint-claim transactions, including the "who reached the top first" race condition.

---

## P2.8 — Atomic Checkpoint-Claim Transaction Endpoint

**Prerequisites / State Check:** `P2.7` complete (same concurrency pattern reused/extended here).

**Objective:** Implement `POST /seasons/{seasonId}/checkpoints/{checkpointIndex}/claim`, handling two distinct race conditions: (a) duplicate/retried claims from the same player (idempotency), and (b) the competitive "first to reach the top" ranking, where exactly one player's claim on the final checkpoint must win the race, even under truly simultaneous requests.

**Expected Output:** A claim endpoint that is idempotent per player/checkpoint, and for the final checkpoint specifically, assigns a correct, gapless, race-safe rank ordering (1st, 2nd, 3rd...) even when multiple claims arrive within the same millisecond.

**Files to Modify:**
- `Backend/MonolithV.Data/CheckpointRepository.cs` (new)
- `Backend/MonolithV.Api/Controllers/CheckpointsController.cs` (new)
- `Backend/MonolithV.Tests/CheckpointClaimConcurrencyTests.cs` (new)
- `Infra/oracle/schema/V3__season_rankings.sql` (new — `SEASON_RANKINGS` table: `SEASON_ID`, `PLAYER_ID`, `RANK`, `CLAIMED_AT`, unique on `(SEASON_ID, RANK)` and on `(SEASON_ID, PLAYER_ID)`)

**Implementation Steps:**
1. For non-final checkpoints: `INSERT` into `CHECKPOINT_PROGRESS` with the same idempotent-via-unique-constraint pattern as P2.7 (duplicate claims are harmless no-ops).
2. For the final checkpoint (the literal "top"): the rank assignment is the actual race condition. Do **not** compute `next rank = COUNT(*) + 1` in application code and then insert separately — that's exactly the check-then-act race that duplicates ranks under concurrency. Instead, use an atomic Oracle sequence (`CREATE SEQUENCE season_rank_seq`) or a single `INSERT ... SELECT NVL(MAX(RANK),0)+1 FROM SEASON_RANKINGS WHERE SEASON_ID = :seasonId FOR UPDATE` pattern (row-locking the relevant rows for the duration of the transaction) so rank assignment and insert happen as one atomic, serialized database operation.
3. Wrap this in a `SERIALIZABLE` (or Oracle's default read-committed plus explicit row locking via `FOR UPDATE`) transaction, and additionally apply the same in-process keyed-semaphore pattern from P2.7 keyed by `seasonId` alone for the final checkpoint (all final-checkpoint claims in a season serialize through one lock, since they all contend for the same rank sequence — this is a deliberate, small, correctness-motivated bottleneck, not a performance one, and is fine at Month-1 concurrency scale).
4. Return the assigned rank in the response so the client can show "You reached the top! Rank: #2 this season."
5. Write `CheckpointClaimConcurrencyTests`: fire N concurrent claims from N distinct fake players against the final checkpoint, assert ranks `1..N` are assigned with no duplicates and no gaps.
6. Commit.

**Validation Checklist:**
- [ ] Duplicate claims from the same player on the same checkpoint are idempotent no-ops
- [ ] N concurrent distinct-player final-checkpoint claims produce exactly the rank set `{1, ..., N}` with no duplicates or gaps
- [ ] Unique constraints on `SEASON_RANKINGS` confirmed present

**Testing Checklist:**
- [ ] `CheckpointClaimConcurrencyTests` passes reliably across 5+ repeated runs with N ≥ 10 concurrent fake claims
- [ ] Manual test: two real Unreal clients race to the top checkpoint, confirm correct 1st/2nd rank assignment matching actual arrival order as closely as network timing allows

**Git Commit Message:**
```
[P2.8] Add atomic checkpoint-claim endpoint with race-safe rank assignment
```

**Documentation Updates:** `Docs/Architecture/Sequence_Diagram.md` — add the checkpoint-claim race-condition sequence; `Docs/SRS.md` — mark `FR` for "Atomic Shared-State Data Engine" as implemented, citing this prompt.

**Common Mistakes:**
- Computing the next rank via a separate `SELECT COUNT(*)` then a separate `INSERT` — this is the textbook check-then-act race condition and will produce duplicate ranks under real concurrency; the select-and-insert (or lock-and-insert) must be one atomic operation.
- Testing with artificial delays between concurrent requests ("staggered" by a few ms in a test loop) — this hides the exact bug you're trying to prove doesn't exist; use true concurrent dispatch (`Task.WhenAll`).

**Next Prompt Dependency:** P2.9 wires the Unreal server (not client) to actually call these two endpoints.

---

## P2.9 — Unreal-to-Backend HTTP Client (Server-Authoritative Calls Only)

**Prerequisites / State Check:** `P2.8` complete. Backend endpoints tested and working via direct HTTP calls (curl/Postman).

**Objective:** Implement the C++ HTTP client wrapper inside the dedicated server (never the game client directly) that calls the share-event and checkpoint-claim backend endpoints, preserving server authority: a client can request a share/claim action via RPC to the server, but only the server talks to the backend and only the server's resulting state is trusted/replicated back.

**Expected Output:** A gameplay action (e.g. a debug console command standing in for the real Phase 3 "share item" interaction) triggers: Client RPC → Server validates game-state preconditions → Server calls backend API asynchronously → Server applies the result to replicated game state once the backend confirms.

**Files to Modify:**
- `Game/Source/MonolithV/Networking/BackendApiClient.h/.cpp` (new, using Unreal's `HttpModule`/`FHttpRequest`)
- `Game/Source/MonolithV/Player/MonolithVCharacter.cpp` (add a server RPC `ServerRequestShareItem`, temporary debug-bound)

**Implementation Steps:**
1. Implement `UBackendApiClient` (a `UGameInstanceSubsystem`, server-side only in practice since only the dedicated server process will actually invoke it in the shipped design) wrapping `FHttpModule::Get().CreateRequest()`, with a method `PostShareEvent(FString SeasonId, FString GiverId, FString ReceiverId, FString ItemType, TFunction<void(bool bSuccess, bool bAlreadyShared)> Callback)` — fully async via Unreal's HTTP delegate callbacks, never blocking the game thread.
2. Add `AMonolithVCharacter::ServerRequestShareItem_Implementation()` (a `Server` RPC), which first validates game-side preconditions (are the two characters in proximity, does the acting player actually hold the item — real logic lands in Phase 3; stub a simple "always true" precondition check for now with a `// TODO Phase 3` marker), then calls `UBackendApiClient::PostShareEvent`.
3. On the HTTP callback, apply the result to a replicated game-state flag (e.g. a temporary `bDebugShareConfirmed` replicated bool) so clients can observe the round-trip completed correctly.
4. Bind a temporary debug key to call `ServerRequestShareItem` for manual testing (removed/replaced by real interaction UI in Phase 3).
5. Test the full loop with 2 clients + dedicated server + backend + Oracle + Redis all running simultaneously (local VM), confirming the debug flag updates correctly on both clients after the backend round-trip.
6. Commit.

**Validation Checklist:**
- [ ] HTTP calls originate only from the server process, never the client directly (verify by checking the client build has no reachable backend base-URL config, or by code review confirming the call site is server-only)
- [ ] Full round trip (Client RPC → Server → Backend → Oracle → Response → Replicated state) completes correctly in a live 2-client test
- [ ] HTTP calls are fully async (no blocking wait on the game thread — verify via `stat game` showing no frame hitches during the call)

**Testing Checklist:**
- [ ] Full-stack manual test as described above, with all 5 components (2 clients, server, backend, DB+cache) running together at least once before moving on

**Git Commit Message:**
```
[P2.9] Wire server-authoritative backend HTTP client for share/claim actions
```

**Documentation Updates:** `Docs/Architecture/Sequence_Diagram.md` — finalize the end-to-end sequence diagram covering client through database and back.

**Common Mistakes:**
- Letting the game **client** call the backend API directly (even "just for now") — this breaks server authority and reopens exactly the tampering vector the abstract's pillar 1 exists to close; the client must only ever RPC its *intent* to the server.
- Blocking the game thread waiting on the HTTP response — always use the async delegate pattern; a synchronous wait here would violate the same "never block the networking thread" principle the backend side already enforces.

**Next Prompt Dependency:** P2.10 adds the season/role assignment flow using this same server→backend call pattern.

---

## P2.10 — Season & Role Assignment Flow

**Prerequisites / State Check:** `P2.9` complete (reuses the `BackendApiClient` pattern).

**Objective:** Implement the actual "player picks male/female role per season" flow end-to-end: client presents the choice, server validates and persists it via a new backend endpoint, writing to `PLAYER_SEASON_ROLES`.

**Expected Output:** On first connecting to a season, a player selects a role (temporary debug UI/console command standing in for the real Phase 3 menu), the choice is persisted, and reconnecting the same player shows their already-assigned role rather than prompting again.

**Files to Modify:**
- `Backend/MonolithV.Api/Controllers/SeasonRolesController.cs` (new — `POST /seasons/{seasonId}/players/{playerId}/role`, `GET /seasons/{seasonId}/players/{playerId}/role`)
- `Backend/MonolithV.Data/SeasonRoleRepository.cs` (new)
- `Game/Source/MonolithV/Player/MonolithVPlayerController.cpp` (server-side: on login, check existing role via GET; if none, request role choice)

**Implementation Steps:**
1. Implement `POST .../role` with the same idempotent-insert pattern (unique PK on `PLAYER_ID, SEASON_ID` already defined in P1.7 schema) — rejects a second differing role choice for the same player/season with `409 Conflict` (role is fixed once chosen for the season, matching your design decision).
2. Implement `GET .../role` returning the existing role or `404` if unset.
3. On `AMonolithVPlayerController::PostLogin` (server-side), call `GET` first; if `404`, hold the player in a "choosing role" state and wait for a client RPC carrying their choice, then `POST` it.
4. Confirm re-running the same player through login twice (disconnect/reconnect) correctly skips the choice prompt the second time (state already persisted).
5. Commit.

**Validation Checklist:**
- [ ] First login prompts for role; persists correctly
- [ ] Second login (same player, same season) does not re-prompt, loads existing role
- [ ] Attempting to change role mid-season is rejected with 409

**Testing Checklist:**
- [ ] Manual test: full disconnect/reconnect cycle for one test player, confirm role persistence across the reconnect

**Git Commit Message:**
```
[P2.10] Implement season role assignment flow with persistence
```

**Documentation Updates:** `Docs/Architecture/Activity_Diagram.md` — update the login flow with the real role-assignment branch (replacing the Phase 1 placeholder version).

**Common Mistakes:**
- Allowing role changes mid-season by simply overwriting the row — this silently breaks the season-long fairness/identity assumption baked into the share-gate design; must be an explicit reject, not a silent overwrite.

**Next Prompt Dependency:** P2.11 adds a baseline server-side validation pass across everything built so far before the integration test in P2.12.

---

## P2.11 — Server-Side Validation Baseline (Anti-Cheat Philosophy)

**Prerequisites / State Check:** P2.1–P2.10 complete.

**Objective:** Explicitly audit and harden every client-originating input path built so far (movement, GAS ability activation, share requests, role choice, checkpoint claims) against the core principle "the server never trusts a client-reported fact, only a client-reported *intent*, which the server independently validates against its own authoritative state" — this is the whole of your anti-cheat strategy, and it should be demonstrable, not just claimed.

**Expected Output:** A short internal audit document plus at least one concrete server-side validation added where it was previously missing (e.g. confirming `ServerRequestShareItem` actually checks proximity/possession server-side before calling the backend, closing the `// TODO Phase 3` stub from P2.9 with at least a minimal real check).

**Files to Modify:**
- `Game/Source/MonolithV/Player/MonolithVCharacter.cpp` (add real proximity/possession validation to `ServerRequestShareItem`)
- `Docs/Architecture/AntiCheatAudit.md` (new)

**Implementation Steps:**
1. List every server RPC / backend-triggering entry point that exists so far, and for each, write one line in `AntiCheatAudit.md`: what the client claims, what the server independently verifies before acting on it.
2. Close the biggest gap concretely: add a server-side distance check (`FVector::Dist` between the two characters' server-authoritative locations) in `ServerRequestShareItem` before it's allowed to proceed, rejecting (and logging) requests claiming a share from an impossible distance.
3. Add a basic rate-limit on `ServerRequestShareItem` (e.g. reject if called again within 1 second of the last call from the same connection) as a simple abuse-prevention measure, logged server-side.
4. Confirm (via the earlier P2.2 test) that movement remains server-corrected, and add one more explicit test: attempt to trigger `GA_TestAbility` from a modified/fake client input pattern (e.g. spamming the input far faster than the ability's cooldown allows) and confirm GAS's own cooldown/cost gating rejects it server-side.
5. Commit.

**Validation Checklist:**
- [ ] `AntiCheatAudit.md` lists every current client-facing entry point with its server-side check
- [ ] Distance check added and tested (attempt a share from far away, confirm rejection)
- [ ] Rate-limit tested (rapid-fire attempts confirmed throttled)

**Testing Checklist:**
- [ ] Manual "cheat attempt" pass: try to break each documented entry point (fake distance, spam input, tamper local position) and confirm each is correctly rejected server-side

**Git Commit Message:**
```
[P2.11] Add server-side validation audit and close share-request trust gap
```

**Documentation Updates:** `Docs/Architecture/AntiCheatAudit.md` created; reference it from `Docs/SRS.md`'s non-functional security requirements section.

**Common Mistakes:**
- Treating "server-authoritative" as a property you get for free just by having a dedicated server — it only holds for the specific paths you've actually validated; this audit exists precisely because it's easy to add a new client-triggered action in Phase 3 and forget to validate it server-side.

**Next Prompt Dependency:** P2.12 is the full-stack integration smoke test proving everything in this phase works together.

---

## P2.12 — Full-Stack Integration Smoke Test

**Prerequisites / State Check:** P2.1–P2.11 complete.

**Objective:** One deliberate end-to-end test run exercising every system built in Phase 2 together — dedicated server, 2 EOS-authenticated clients, movement replication, GAS ability, altitude streaming across bands, role assignment, a share-event, and a checkpoint claim — with results recorded as the formal close-out evidence for this phase.

**Expected Output:** A written test log in `Docs/Testing/TestPlan.md` under "Phase 2 Integration Test" documenting each step performed and its actual observed result (pass/fail per system), plus a short screen-recorded demo clip.

**Files to Modify:**
- `Docs/Testing/TestPlan.md`

**Implementation Steps:**
1. Start: local Linux VM running Oracle-connected backend + Redis (Docker Compose) + packaged `MonolithVServer` binary.
2. Two client instances (can be two packaged Development client builds, or one packaged + one PIE instance) log in via EOS dev-auth, find/join the session.
3. Both move around (P2.2/P2.3), confirm smooth replicated movement.
4. Both activate the test GAS ability, confirm replicated health changes visible to both.
5. Move one player through all 5 altitude bands, confirm streaming transitions and record `stat memory` before/after.
6. Both players complete season role assignment (opposite roles).
7. Trigger a share-event between them via the debug-bound action, confirm success response and correct DB row (`SELECT * FROM SHARE_EVENTS` via SQLcl).
8. Both players claim checkpoints, including the final checkpoint, confirm correct rank assignment in `SEASON_RANKINGS`.
9. Record every step's pass/fail in `Docs/Testing/TestPlan.md`, with any failures filed as follow-up notes (fix before proceeding to Phase 3 if a failure blocks core functionality; log as a known issue in `PROJECT_STATE.md` if minor/non-blocking).
10. Commit the test log.

**Validation Checklist:**
- [ ] Every one of the 8 test steps above passes
- [ ] Any failures are either fixed or explicitly logged as known issues before Phase 3 begins

**Testing Checklist:**
- [ ] This prompt's entire implementation IS the testing checklist — record actual results, not intended results

**Git Commit Message:**
```
[P2.12] Complete Phase 2 full-stack integration smoke test
```

**Documentation Updates:** `Docs/Testing/TestPlan.md` — "Phase 2 Integration Test" section complete with real results.

**Common Mistakes:**
- Skipping steps that "obviously work" from earlier individual prompt testing — the point of this prompt is specifically to catch integration failures that don't show up when systems are tested in isolation (e.g. the streaming manager and the GAS ability system have never actually run at the same time before this test).

**Next Prompt Dependency:** This is the last prompt of Phase 2. Update `PROJECT_STATE.md`: `Last Completed Prompt ID: P2.12`, `Current Phase: Phase 2 complete → Phase 3`, `Next Prompt To Run: P3.1`. Tag `phase-2-complete`, merge `develop` → `main`. Strong Twitter/X milestone moment: two players moving, fighting, climbing, and racing to a real database-backed checkpoint — your first genuinely multiplayer clip.
