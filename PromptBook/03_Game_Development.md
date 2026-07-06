# Phase 3 — Game Development

Goal of this phase: turn the proven Phase 2 plumbing into an actual playable vertical slice — real role selection, the golden apple/counterpart share-gate mechanic, jetpack traversal, hybrid gun+ability combat, guardian AI spawning from the Monolith, checkpoints/respawn, one fully themed level plus one greybox level proving the theming system scales, HUD, and a basic season/leaderboard readout. By the end of this phase, a stranger can sit down and actually play Monolith-V.

---

## P3.1 — Role Selection UI (Male/Female)

**Prerequisites / State Check:** `PROJECT_STATE.md` → `Last Completed Prompt ID: P2.12`. Backend role endpoints (P2.10) working.

**Objective:** Replace the P2.10 debug-stub role choice with a real UMG menu shown on first login each season, calling the same server RPC → backend flow already proven in Phase 2.

**Expected Output:** A clean two-option (Male/Female) UMG widget appears exactly once per player per season, disables itself after a choice is made, and the choice visibly affects the character (e.g. a distinct held-item silhouette or accessory) for the rest of the session.

**Files to Modify:**
- `Game/Content/UI/WBP_RoleSelect` (new UMG widget)
- `Game/Source/MonolithV/UI/RoleSelectWidget.h/.cpp` (new C++ base class for the widget)
- `Game/Source/MonolithV/Player/MonolithVPlayerController.cpp` (show widget instead of debug stub)

**Implementation Steps:**
1. Create `URoleSelectWidget : public UUserWidget` in C++ with two `UButton`s bound in a child UMG Blueprint (`WBP_RoleSelect`), each calling a C++-exposed `SelectRole(EPlayerRole Role)` function.
2. `SelectRole` calls a `Server_SubmitRoleChoice` RPC on the owning `PlayerController`, which performs the P2.10 backend `POST`.
3. On successful backend response, close the widget and set a replicated `EPlayerRole` enum on the character (used in P3.2 to determine which item they carry).
4. Update `PostLogin` logic from P2.10: if `GET` returns 404, show `WBP_RoleSelect` (instead of the old debug console flow); if role exists, skip straight to gameplay.
5. Add simple, clear widget styling (large buttons, role icon placeholders — real art in P3.11) consistent with the stylized art direction.
6. Test full flow with 2 clients choosing opposite roles.
7. Commit.

**Validation Checklist:**
- [ ] Widget appears exactly once per player per season
- [ ] Choice persists correctly (matches P2.10 backend behavior)
- [ ] Character visibly reflects chosen role after selection

**Testing Checklist:**
- [ ] Reconnect test: player who already chose a role does not see the widget again

**Git Commit Message:**
```
[P3.1] Add real role-selection UI replacing debug stub
```

**Documentation Updates:** `Docs/UserManual.md` — "Choosing Your Role" section with a screenshot.

**Common Mistakes:**
- Doing the backend call directly from the widget instead of routing through the PlayerController's server RPC — widgets are client-side UI only; the actual authoritative action must go through the same server-validated path as everything else in this project.

**Next Prompt Dependency:** P3.2 uses the replicated `EPlayerRole` this prompt introduces.

---

## P3.2 — Golden Apple & Counterpart Item

**Prerequisites / State Check:** `P3.1` complete — `EPlayerRole` replicated on character.

**Objective:** Implement the two role-locked items — Golden Apple (male) and its symmetric counterpart (design it now, e.g. a "Silver Blossom" that grants the same kind of ally-health-share effect) — as real pickup-or-starting-inventory items with a "share with nearby opposite-role ally" interaction that calls the P2.7 backend endpoint.

**Expected Output:** Each player spawns holding their role's item; standing near an opposite-role player and pressing the share-interact key triggers the server RPC → backend share-event call from Phase 2, applies a real gameplay effect (temporary health boost, via a `GameplayEffect` on both participants), and marks the item consumed/on-cooldown.

**Files to Modify:**
- `Game/Source/MonolithV/Combat/RoleItemComponent.h/.cpp` (new — attached to character, holds item state)
- `Game/Source/MonolithV/Combat/GE_ShareHealthBoost.h/.cpp` (new `GameplayEffect`)
- `Game/Source/MonolithV/Player/MonolithVCharacter.cpp` (add `ServerRequestShareItem` real implementation, replacing the P2.9 stub's `// TODO Phase 3`)

**Implementation Steps:**
1. Add `URoleItemComponent` to `AMonolithVCharacter`, tracking `bool bItemAvailable` (replicated), initialized `true` on spawn, item type derived from `EPlayerRole`.
2. Implement the real precondition checks in `ServerRequestShareItem_Implementation` (closing the P2.9/P2.11 stub properly): distance check (already added in P2.11), opposite-role check, `bItemAvailable` check on the giver, and that the receiver hasn't already received a share from this giver this season (mirrors the backend's own idempotency, checked client-experience-side first to give immediate feedback before the network round trip).
3. On passing all checks, call `UBackendApiClient::PostShareEvent` (P2.9); on success (not `alreadyShared`), apply `GE_ShareHealthBoost` to both the giver and receiver via their `AbilitySystemComponent`s (a temporary `MaxHealth` buff, duration-based `GameplayEffect`), and set `bItemAvailable = false` on the giver for the rest of the season (one-time use, matching the "required for late-game mechanic" design).
4. Add a simple world-space or HUD prompt ("Press F to share Golden Apple with [Name]") shown only when in range of a valid opposite-role target with an available item.
5. Test the full loop with 2 clients of opposite roles: approach, share, confirm both receive the buff, confirm the giver's item shows consumed and a second share attempt from the same giver correctly fails as "already used."
6. Commit.

**Validation Checklist:**
- [ ] Share interaction only appears for valid opposite-role, in-range, available-item targets
- [ ] Successful share applies the health buff to both participants
- [ ] Item is correctly marked consumed after use and cannot be reused

**Testing Checklist:**
- [ ] Two clients, full share interaction, confirm buff visually/numerically on both
- [ ] Attempt a same-role "share" and confirm it's correctly blocked with appropriate feedback

**Git Commit Message:**
```
[P3.2] Implement Golden Apple / counterpart item share mechanic
```

**Documentation Updates:** `Docs/UserManual.md` — "Sharing Items" section; `Docs/SRS.md` — mark the role-item FR as fully implemented.

**Common Mistakes:**
- Applying the health buff optimistically on the client before backend confirmation — always wait for the server's confirmed backend response before applying any gameplay effect, preserving the authority chain established in Phase 2.

**Next Prompt Dependency:** P3.3 builds the checkpoint gate that specifically requires this share to have happened.

---

## P3.3 — Share-Gated Checkpoint Mechanic

**Prerequisites / State Check:** `P3.2` complete. P2.8 checkpoint-claim endpoint working.

**Objective:** Implement the actual late-tower gate: a specific checkpoint (e.g. the entrance to the top band) that will not unlock for a player unless a `SHARE_EVENTS` row exists involving them and some opposite-role player this season — querying the backend, not re-deriving this logic client-side.

**Expected Output:** A visible, in-world gate/barrier actor at the gated checkpoint that only opens (disables its blocking collision, plays an unlock effect) once the approaching player's share-status check succeeds; otherwise it stays shut with clear player feedback on what's missing.

**Files to Modify:**
- `Backend/MonolithV.Api/Controllers/ShareEventsController.cs` (add `GET /seasons/{seasonId}/players/{playerId}/has-shared`)
- `Game/Source/MonolithV/World/ShareGateActor.h/.cpp` (new)

**Implementation Steps:**
1. Add the backend `GET has-shared` endpoint: returns `true` if any `SHARE_EVENTS` row exists this season where the player is either giver or receiver (a single indexed query against the index added in P1.7).
2. Implement `AShareGateActor` with a blocking collision volume; on `BeginOverlap` by a player pawn, the server calls `UBackendApiClient::GetHasShared(playerId, seasonId, callback)`.
3. On `true`, server disables the gate's collision for that specific player (or globally opens it, depending on your design choice — recommend per-player pass-through via a temporary ignore-collision response rather than a permanently-open world gate, since different players reach it having shared or not) and replicates a "gate open" cosmetic state.
4. On `false`, keep the gate blocking and trigger client-side feedback (HUD message: "You must share your [item] with an ally before passing").
5. Test with one player who has shared (passes) and one who hasn't (blocked), confirming correct per-player behavior.
6. Commit.

**Validation Checklist:**
- [ ] Player who has shared passes the gate
- [ ] Player who hasn't shared is blocked with clear feedback
- [ ] Gate check is server-authoritative (client cannot force passage)

**Testing Checklist:**
- [ ] Both positive and negative cases tested live with 2 clients in one session

**Git Commit Message:**
```
[P3.3] Implement share-gated checkpoint mechanic
```

**Documentation Updates:** `Docs/Architecture/Flowchart.md` — this is exactly the flowchart drafted in P1.14; update it to reflect the real implementation.

**Common Mistakes:**
- Making the gate open globally (for all players) once any one player shares — the design intent is per-player passage validated against that specific player's own share history, not a shared world-state flag.

**Next Prompt Dependency:** P3.4 builds jetpack traversal, needed to actually reach this and other altitude bands.

---

## P3.4 — Jetpack Traversal Ability

**Prerequisites / State Check:** `P3.3` complete. GAS skeleton from P2.4 in place.

**Objective:** Implement the jetpack as a real GAS ability granting upward thrust with a fuel/cooldown resource, replicated and server-validated, as the primary means of gaining altitude between bands.

**Expected Output:** Holding the jetpack input applies upward force via `UCharacterMovementComponent` (through `AddImpulse`/`AddMovementInput` with an upward vector, respecting existing prediction), consuming a `Fuel` GAS attribute that regenerates over time when not in use, replicated correctly to all clients.

**Files to Modify:**
- `Game/Source/MonolithV/Combat/GA_Jetpack.h/.cpp` (new)
- `Game/Source/MonolithV/Combat/MonolithVAttributeSet.h/.cpp` (add `Fuel`/`MaxFuel` attributes)
- `Game/Source/MonolithV/Player/MonolithVCharacter.cpp` (grant ability on spawn, bind input)

**Implementation Steps:**
1. Add `Fuel`/`MaxFuel` `FGameplayAttributeData` to `MonolithVAttributeSet`, same replication pattern as `Health`.
2. Create `GA_Jetpack` as a GAS ability with `InstancingPolicy = InstancedPerActor`, activated on input-held (not just pressed), applying a per-tick `GameplayEffect` (a "periodic" instant cost effect) draining `Fuel` while held, and ending the ability automatically when `Fuel` reaches 0.
3. Inside the ability's tick/execution, call `GetCharacterMovement()->AddImpulse(FVector::UpVector * ThrustStrength, true)` or set `CharacterMovement->Velocity.Z` directly within GAS's server-authoritative execution (never from a raw client-side input handler bypassing GAS).
4. Add a passive `Fuel` regeneration `GameplayEffect` (periodic, small positive delta) active whenever the jetpack ability isn't currently running.
5. Add a fuel gauge to the temporary debug HUD (real HUD polish in P3.10).
6. Test: player holds jetpack, gains altitude, fuel depletes, ability auto-cancels at 0 fuel, fuel regenerates when released, tested across the P2.5 altitude bands to confirm streaming still triggers correctly during jetpack-driven ascent (faster Z-velocity than walking might stress the 1Hz band-check throttle — verify no band is skipped).
7. Commit.

**Validation Checklist:**
- [ ] Jetpack ability replicates correctly to all clients (visible thrust effect/animation stub on all observers)
- [ ] Fuel drains and regenerates correctly, is server-authoritative (attempt to fake infinite fuel client-side and confirm rejection)
- [ ] Band-streaming from P2.5 correctly keeps up with jetpack-speed ascent (no band skipped even at max thrust speed — adjust the 1Hz throttle or load-buffer of ±1 band if needed)

**Testing Checklist:**
- [ ] Full ascent test from band 0 to band 4 via jetpack only, confirm correct streaming and no fall-through-world or unloaded-collision issues

**Git Commit Message:**
```
[P3.4] Implement jetpack traversal ability with fuel resource
```

**Documentation Updates:** `Docs/Architecture/Class_Diagram.md` — add `GA_Jetpack`, `Fuel`/`MaxFuel` attributes.

**Common Mistakes:**
- Applying jetpack thrust in `Tick()` directly on the character instead of through the GAS ability's execution — bypasses cost/cooldown gating and reopens a client-trust gap identical to the one closed in P2.11.
- Not re-testing altitude streaming after adding a much faster ascent method — the P2.5 band-check throttle was tuned against walking speed; verify it still holds.

**Next Prompt Dependency:** P3.5 adds the weapon side of "hybrid gun + ability" combat.

---

## P3.5 — Ranged Weapon System

**Prerequisites / State Check:** `P3.4` complete.

**Objective:** Implement one functional ranged weapon (hit-scan, simplest correct choice for a 1-month solo scope over full projectile simulation) dealing damage via a `GameplayEffect`, fired via a GAS ability, validated server-side.

**Expected Output:** Pressing fire triggers a server-authoritative hit-scan trace from the weapon's muzzle, applying `GE_WeaponDamage` to whatever it hits (guardian or player), replicated correctly, with basic ammo/cooldown.

**Files to Modify:**
- `Game/Source/MonolithV/Combat/WeaponComponent.h/.cpp` (new)
- `Game/Source/MonolithV/Combat/GA_FireWeapon.h/.cpp` (new)
- `Game/Source/MonolithV/Combat/GE_WeaponDamage.h/.cpp` (new)

**Implementation Steps:**
1. Add `UWeaponComponent` to the character, holding `Ammo`/`MaxAmmo`, `FireRate`, `Range`, `Damage`.
2. Implement `GA_FireWeapon`: on activation, server performs `UWorld::LineTraceSingleByChannel` from the camera/muzzle socket out to `Range`, using a dedicated `ECC_GameTraceChannel` for "damageable" actors.
3. On a valid hit implementing `IAbilitySystemInterface`, apply `GE_WeaponDamage` (an instant `GameplayEffect` subtracting `Damage` from the target's `Health`) via `AbilitySystemComponent->ApplyGameplayEffectToTarget`.
4. Gate firing on `Ammo > 0` and a cooldown `GameplayEffect` (standard GAS cooldown tag pattern), decrementing `Ammo` per shot; add a simple reload input restoring `Ammo` to `MaxAmmo` after a delay.
5. Add hit-feedback: a simple particle/decal stub at the trace impact point (real VFX polish is optional given the 1-month budget — a basic `NiagaraSystem` stub is enough to prove the pipeline).
6. Test damage application against another player character (killing/checking respawn comes in P3.9) and confirm cooldown/ammo gating is server-enforced (attempt rapid client-side fire-spam and confirm the cooldown effect blocks it).
7. Commit.

**Validation Checklist:**
- [ ] Hit-scan trace is server-side, not client-side-then-trusted
- [ ] Damage applies correctly and replicates
- [ ] Ammo/cooldown gating is server-enforced against a spam-fire attempt

**Testing Checklist:**
- [ ] Two clients, one shoots the other, confirm correct damage application and `Health` replication observed by both

**Git Commit Message:**
```
[P3.5] Implement server-authoritative ranged weapon system
```

**Documentation Updates:** `Docs/Architecture/Class_Diagram.md` — add `WeaponComponent`, `GA_FireWeapon`, `GE_WeaponDamage`.

**Common Mistakes:**
- Performing the hit-scan trace on the client and sending "I hit X" to the server as a trusted fact — the trace itself must run server-side against server-authoritative actor positions; client-side traces (if used at all) are only for immediate visual feedback, never the authority.

**Next Prompt Dependency:** P3.6 adds one more combat/traversal ability to round out "hybrid" combat.

---

## P3.6 — Secondary Combat Ability

**Prerequisites / State Check:** `P3.5` complete.

**Objective:** Implement one additional GAS ability beyond the weapon and jetpack, giving combat real hybrid texture — a jetpack-charged dash-attack is recommended (reuses jetpack fuel as a resource, ties traversal and combat together thematically and mechanically, and is cheap to implement given P3.4/P3.5 already exist).

**Expected Output:** A `GA_DashAttack` ability consuming a fixed `Fuel` cost, propelling the character forward rapidly and applying `GE_WeaponDamage`-style damage to anything struck during the dash (via a capsule sweep, not a hit-scan trace).

**Files to Modify:**
- `Game/Source/MonolithV/Combat/GA_DashAttack.h/.cpp` (new)

**Implementation Steps:**
1. Create `GA_DashAttack`, cost = fixed `Fuel` amount (checked via a GAS cost `GameplayEffect`, consistent with P3.4's resource), cooldown = a few seconds (GAS cooldown tag pattern, consistent with P3.5).
2. On activation, server performs a capsule sweep (`UWorld::SweepMultiByChannel`) along the character's forward vector over a short distance/duration, launching the character forward (`LaunchCharacter`) simultaneously.
3. Apply a `GameplayEffect` (can reuse `GE_WeaponDamage` with a different magnitude, or a new `GE_DashDamage`) to every valid hit target along the sweep.
4. Test resource sharing correctness: confirm using `GA_Jetpack` and `GA_DashAttack` back-to-back correctly reflects combined `Fuel` drain (no double-counting or missed cost application from having two abilities pull from the same attribute).
5. Commit.

**Validation Checklist:**
- [ ] Dash-attack costs Fuel correctly and respects cooldown
- [ ] Sweep correctly hits and damages targets along the dash path
- [ ] Fuel attribute behaves correctly when both jetpack and dash-attack are used in the same session (shared resource, no double-drain bugs)

**Testing Checklist:**
- [ ] Two clients, one dash-attacks the other, confirm damage and knockback observed correctly by both

**Git Commit Message:**
```
[P3.6] Implement dash-attack ability sharing the jetpack fuel resource
```

**Documentation Updates:** `Docs/Architecture/Class_Diagram.md` — add `GA_DashAttack`.

**Common Mistakes:**
- Giving the dash-attack its own separate resource instead of sharing `Fuel` — this was a deliberate design choice to make jetpack/combat resource management interesting; keep them coupled.

**Next Prompt Dependency:** P3.7 builds the guardian AI this combat kit will actually be used against.

---

## P3.7 — Guardian AI (Behavior Tree)

**Prerequisites / State Check:** `P3.6` complete.

**Objective:** Implement one real guardian enemy archetype: a `Behavior Tree`-driven AI that detects players, approaches, and attacks using the same GAS/weapon pattern as players (a guardian is just another `AbilitySystemComponent`-having actor, for consistency and code reuse), server-authoritative like everything else.

**Expected Output:** A guardian pawn that patrols/idles, detects a player within a perception radius, chases, and attacks (melee swipe or a simple projectile) dealing real damage via GAS, killable via player weapons/abilities.

**Files to Modify:**
- `Game/Source/MonolithV/AI/GuardianCharacter.h/.cpp` (new)
- `Game/Source/MonolithV/AI/GuardianAIController.h/.cpp` (new)
- `Game/Content/AI/BT_Guardian`, `BB_Guardian` (new Behavior Tree/Blackboard assets)
- `Game/Source/MonolithV/AI/BTTask_AttackPlayer.h/.cpp` (new custom BT task)

**Implementation Steps:**
1. Create `AGuardianCharacter : public ACharacter`, implementing `IAbilitySystemInterface` with its own `AttributeSet` (reuse `UMonolithVAttributeSet` or a `UGuardianAttributeSet` subset with just `Health`).
2. Create `AGuardianAIController : public AAIController` with a `UAIPerceptionComponent` (sight sense, reasonable radius/FOV for the scale of your test arena).
3. Build `BB_Guardian` blackboard with keys: `TargetActor` (object), `HasLineOfSight` (bool).
4. Build `BT_Guardian`: root → Selector → [Sequence: perceive → chase → attack] with a fallback Idle/Patrol branch when no target is perceived.
5. Implement `BTTask_AttackPlayer`, applying a `GameplayEffect` damage instance to the perceived target when in range, on a cooldown (identical pattern to player abilities — this consistency is worth calling out in your report as good architecture).
6. Ensure `AGuardianCharacter` also uses `UCharacterMovementComponent` for its chase movement, so it is replicated using the exact same server-authoritative movement pattern as players (no separate/duplicate movement replication system).
7. Test: guardian idles, detects an approaching player, chases, attacks, is damageable and killable by the player's weapon (P3.5) or dash-attack (P3.6), confirm death removes/respawns the guardian appropriately (simple destroy + respawn-after-delay is sufficient for Month 1).
8. Commit.

**Validation Checklist:**
- [ ] Guardian correctly detects, chases, and attacks a player within perception range
- [ ] Guardian is damageable by both player weapon and dash-attack, dies at 0 Health
- [ ] Guardian movement/combat is server-authoritative (same validation standard as player systems)

**Testing Checklist:**
- [ ] Solo playtest: engage and defeat one guardian start-to-finish
- [ ] Two-client test: confirm both players see the same guardian state (health, position) consistently

**Git Commit Message:**
```
[P3.7] Implement guardian AI with behavior tree combat
```

**Documentation Updates:** `Docs/Architecture/Class_Diagram.md` — add `GuardianCharacter`, `GuardianAIController`; `Docs/Architecture/` add a short Behavior Tree diagram/screenshot.

**Common Mistakes:**
- Giving the guardian a bespoke, non-GAS damage system "because it's simpler" — this creates two parallel combat systems to maintain and test instead of one, and undermines the "atomic shared-state" and "server authority" claims being made uniformly across the project.

**Next Prompt Dependency:** P3.8 ties guardian spawning to the Monolith and altitude bands.

---

## P3.8 — Guardian Spawner Tied to the Monolith & Altitude Bands

**Prerequisites / State Check:** `P3.7` complete. `AAltitudeStreamingManager` from P2.5 available.

**Objective:** Implement the actual design fiction — guardians (and their equipment drops) emerge from the Monolith outward into the world, spawn density/difficulty scaling per altitude band, rather than guardians simply being hand-placed in the level.

**Expected Output:** A `AMonolithActor` at the world's center that periodically spawns `AGuardianCharacter` instances into whichever altitude band(s) currently have a player present (server-driven, reading the same per-player band tracking from P2.5), with spawn count/guardian toughness scaling by band index.

**Files to Modify:**
- `Game/Source/MonolithV/World/MonolithActor.h/.cpp` (new)
- `Game/Source/MonolithV/World/AltitudeStreamingManager.h/.cpp` (expose current bands-with-players to the spawner)

**Implementation Steps:**
1. Create `AMonolithActor`, holding a `TArray<FGuardianSpawnConfig>` (one entry per band: max concurrent guardians, guardian "tier" multiplier applied to `MaxHealth`/`Damage` via a simple scalar on spawn).
2. On a timer (independent of the 1Hz streaming check — a slower cadence, e.g. every 10s, is appropriate for spawning), query `AAltitudeStreamingManager` for currently-loaded/occupied bands, and for each, spawn guardians up to that band's configured max if below it, at spawn points sampled from a `NavMesh`-validated location within that band's sublevel.
3. Scale each spawned guardian's `MaxHealth`/`Damage` attributes by the band's tier multiplier on `BeginPlay` (server-side only, before replication of initial state).
4. Ensure guardians despawn/are destroyed if their band unloads (a player leaves the area) — tie guardian lifetime cleanup into the same unload event from P2.5's streaming manager, so memory used by "band N's guardians" is freed exactly when band N itself unloads (this directly reinforces pillar 2's memory-optimization claim — guardians are part of what "unloading a band" actually frees).
5. Test across all 5 bands: confirm scaling difficulty is perceptible (band 4 guardians are noticeably tougher than band 0), and confirm guardians correctly despawn when their band unloads.
6. Commit.

**Validation Checklist:**
- [ ] Guardians spawn only in bands with a present player, scaled correctly by tier
- [ ] Guardians are cleaned up when their band unloads (verify no orphaned actors remain via `stat objects` guardian-class count check before/after unload)
- [ ] Spawn locations are NavMesh-valid (no guardians stuck in geometry)

**Testing Checklist:**
- [ ] Full ascent playtest through all 5 bands, confirming appropriately scaled guardian encounters at each

**Git Commit Message:**
```
[P3.8] Implement Monolith-driven guardian spawner with per-band scaling
```

**Documentation Updates:** `Docs/Architecture/ChunkStreaming.md` (from P2.5) — update to include guardian lifecycle tied to band load/unload.

**Common Mistakes:**
- Spawning guardians independent of the streaming manager's band-occupancy data (e.g. a naive global timer spawning everywhere) — this defeats the entire point of tying pillar 2 and pillar 3-adjacent gameplay together and can spawn guardians in unloaded/invisible bands.

**Next Prompt Dependency:** P3.9 implements the death/respawn loop players will now actually need against these guardians.

---

## P3.9 — Checkpoint Respawn System

**Prerequisites / State Check:** `P3.8` complete. `CHECKPOINT_PROGRESS` claiming (P2.8) working.

**Objective:** Implement the actual death handling: on `Health` reaching 0, the player respawns at their most recently claimed checkpoint (queried from the backend, not just a locally-remembered value, so this survives reconnects too), with `Health`/`Fuel` reset.

**Expected Output:** Dying to a guardian or another player triggers a short death state, then respawn at the correct checkpoint location (looked up via `GET /seasons/{seasonId}/players/{playerId}/checkpoints/latest`), with attributes reset to full.

**Files to Modify:**
- `Backend/MonolithV.Api/Controllers/CheckpointsController.cs` (add `GET .../checkpoints/latest`)
- `Game/Source/MonolithV/Player/MonolithVCharacter.cpp` (`OnRep_Health` triggers death at 0; respawn logic)
- `Game/Source/MonolithV/World/CheckpointActor.h/.cpp` (new — marks physical respawn locations, one per checkpoint index, matched to the band layout)

**Implementation Steps:**
1. Add the backend `GET latest` endpoint returning the highest `CHECKPOINT_INDEX` claimed by the player this season (simple `MAX()` query, safe without special concurrency handling since it's read-only).
2. Place `ACheckpointActor` instances in the world at each checkpoint location (including the P3.3 share-gated one), each tagged with its `CheckpointIndex`, matching the backend's indexing.
3. On `AMonolithVAttributeSet::PostGameplayEffectExecute` detecting `Health <= 0`: server disables the character's collision/movement briefly (a short "defeated" state), calls the `GET latest` endpoint, finds the matching `ACheckpointActor` in the world, teleports (`SetActorLocation`) the character there, then applies a `GameplayEffect` resetting `Health`/`Fuel` to max.
4. Add simple death feedback (camera fade, respawn message) via a client RPC triggered from the server's death-handling flow.
5. Test: die to a guardian, confirm correct respawn at the last claimed checkpoint (test with checkpoints at multiple indices, not just index 0, to prove the lookup is real and not hardcoded).
6. Commit.

**Validation Checklist:**
- [ ] Death correctly triggers at `Health <= 0`
- [ ] Respawn location matches the player's actual latest claimed checkpoint (tested at 2+ different checkpoint indices)
- [ ] Attributes reset correctly on respawn

**Testing Checklist:**
- [ ] Die before claiming any checkpoint (respawns at a sensible default, e.g. checkpoint 0 / world spawn) and die after claiming checkpoint 2 (respawns at checkpoint 2) — both cases tested explicitly

**Git Commit Message:**
```
[P3.9] Implement checkpoint-based respawn system
```

**Documentation Updates:** `Docs/UserManual.md` — "Death & Respawn" section.

**Common Mistakes:**
- Caching the "latest checkpoint" only in a local replicated variable without ever re-querying the backend — this breaks correctness across reconnects/crashes; the backend row is the source of truth, a local cache is only a convenience to avoid a redundant call immediately after claiming.

**Next Prompt Dependency:** P3.10 builds the real HUD surfacing all this state to the player.

---

## P3.10 — HUD & UI Polish

**Prerequisites / State Check:** `P3.9` complete. All debug on-screen messages from earlier prompts (P2.5 band debug, P3.4 fuel gauge debug) still in place as placeholders.

**Objective:** Replace every temporary debug on-screen message with a real UMG HUD: health bar, fuel gauge, current altitude/band indicator, role-item status (available/used), and a directional indicator toward the Monolith.

**Files to Modify:**
- `Game/Content/UI/WBP_HUD` (new)
- `Game/Source/MonolithV/UI/MonolithVHUD.h/.cpp` (new `AHUD` subclass, or a UMG-driven `HUDWidget` approach)
- Remove now-redundant `GEngine->AddOnScreenDebugMessage` calls from P2.5/P3.4

**Implementation Steps:**
1. Build `WBP_HUD` with bound C++ properties reading `Health`/`MaxHealth`, `Fuel`/`MaxFuel` (via GAS attribute change delegates, not polling every tick), current band index (from a new lightweight client-visible replicated property on the character mirroring the server's per-player band tracking), and `bItemAvailable`.
2. Bind GAS attribute-changed delegates (`GetGameplayAttributeValueChangeDelegate`) to update health/fuel bars reactively instead of polling — this is the correct GAS-idiomatic pattern and avoids unnecessary per-tick UI updates.
3. Add a simple directional compass/arrow widget pointing toward `AMonolithActor`'s location (basic vector-math-to-screen-space widget rotation, not a full minimap).
4. Remove the P2.5 and P3.4 debug on-screen messages now that real UI covers the same information.
5. Playtest the full loop once with only the real HUD visible (no debug overlays) to confirm nothing important was lost.
6. Commit.

**Validation Checklist:**
- [ ] Health/fuel bars update reactively and correctly
- [ ] Band indicator and Monolith-direction indicator are correct at multiple test locations
- [ ] All temporary debug overlays removed

**Testing Checklist:**
- [ ] Full playtest relying only on the real HUD, confirming it's sufficient to play without the debug overlays

**Git Commit Message:**
```
[P3.10] Replace debug overlays with polished HUD
```

**Documentation Updates:** `Docs/UserManual.md` — add a HUD explanation screenshot/diagram.

**Common Mistakes:**
- Polling attribute values every `Tick` in the widget instead of binding to GAS's attribute-change delegates — wastes cycles and is the non-idiomatic approach GAS specifically provides delegates to avoid.

**Next Prompt Dependency:** P3.11 builds the real art/level content this HUD and all these systems will be experienced through.

---

## P3.11 — Themed Level 1 (Full Art Pass)

**Prerequisites / State Check:** `P3.10` complete. Blender (Advanced skill level) available for custom hero assets.

**Objective:** Replace the greyboxed 5-band arena from P2.5 with a real, stylized (Genshin/Valorant-adjacent) themed level — pick one theme (e.g. "Shattered Sky Ruins" or similar) for this first fully-realized level, applying custom art to hero elements (Monolith, guardian, checkpoint gate) and marketplace/Quixel assets for filler environment geometry to stay within the 1-month budget.

**Expected Output:** A visually coherent, stylized 5-band level around the Monolith, fully playable with all Phase 2/3 systems intact, looking like a real game rather than a greybox.

**Files to Modify:**
- `Game/Content/Maps/Band_00` … `Band_04` (art pass on existing greybox sublevels)
- `Game/Content/Art/Monolith/`, `Game/Content/Art/Guardian/` (new custom-modeled assets)
- `Game/Content/Materials/` (stylized material pass — desaturated base + a few saturated accent colors, consistent with the Genshin/Valorant reference)

**Implementation Steps:**
1. Block out the final layout within each band (informed by actual playtesting from P3.8/P3.9 — adjust geometry for real traversal/combat flow, don't just re-skin the exact greybox shapes).
2. Model/texture the Monolith itself and the guardian archetype as custom hero assets in Blender (leverage your advanced skill here — these are the two things every screenshot/demo clip will focus on).
3. Source environment filler (rocks, structures, foliage) from Quixel Megascans (free, integrated in UE5) or the Fab/UE Marketplace, re-textured/tinted to match your stylized palette rather than left at default realistic PBR looks.
4. Set up lighting (Lumen is fine at this small scale — UE5's dynamic GI works well with simple stylized geometry and doesn't require the baking pipeline time cost).
5. Full playtest of every Phase 2/3 system inside the finished level (movement, jetpack, weapon, dash, guardian AI, share-gate, checkpoints/respawn, HUD) — confirm nothing broke when moving off the greybox geometry (NavMesh regeneration for guardians is the most likely casualty — regenerate and re-test guardian pathing explicitly).
6. Commit (art assets — be mindful of file sizes; use Git LFS if any asset pushes the repo size significantly, and document that decision in `Infra/oracle/README.md`-adjacent or a new `Docs/ArtPipeline.md`).

**Validation Checklist:**
- [ ] All 5 bands visually themed and coherent
- [ ] Guardian NavMesh regenerated and pathing verified in the final geometry
- [ ] Full system playtest (every Phase 2/3 mechanic) passes in the finished level

**Testing Checklist:**
- [ ] Record the "before/after" comparison (greybox vs. finished) — strong presentation and Twitter/X material

**Git Commit Message:**
```
[P3.11] Complete art pass on Level 1 (themed 5-band arena)
```

**Documentation Updates:** `Docs/ArtPipeline.md` (new) — document asset sourcing (custom vs. marketplace), any Git LFS usage, and the stylized-palette rationale.

**Common Mistakes:**
- Re-skinning materials without re-testing NavMesh/collision — final art geometry often differs enough from greybox collision that guardian pathing silently breaks; always re-test, don't assume.
- Scope creep here — this is the single riskiest prompt for a solo 1-month timeline to overrun; time-box it and prefer "good and finished" over "perfect and half-done."

**Next Prompt Dependency:** P3.12 proves the theming system scales by greyboxing a second, differently-themed level using the same code.

---

## P3.12 — Themed Level 2 (Greybox, Proves Scalability)

**Prerequisites / State Check:** `P3.11` complete.

**Objective:** Prove the "infinitely scalable, differently-themed-per-level" architecture claim concretely: greybox a second level with a distinctly different theme (e.g. "Frozen Spire" vs. Level 1's theme), using only new sublevel data plugged into the existing `AAltitudeStreamingManager`/`AMonolithActor` band configuration — zero new C++ code required.

**Expected Output:** A second, distinctly-themed greybox level (bands 5-9, i.e. "Level 2" per your 5-floors-per-level design) fully functional with every existing system, added purely by extending data (new `FAltitudeBand` entries, new sublevels, new `FGuardianSpawnConfig` entries) — proving the scalability claim isn't just asserted, it's demonstrated.

**Files to Modify:**
- `Game/Content/Maps/Band_05` … `Band_09` (new greybox sublevels)
- Data-only changes to `AAltitudeStreamingManager`'s band array and `AMonolithActor`'s spawn config (via editor-exposed properties/data asset, not hardcoded C++ changes)

**Implementation Steps:**
1. If band/spawn configuration is still hardcoded in C++ from P2.5/P3.8, refactor it now into a `UDataAsset` (e.g. `UAltitudeBandDataAsset`) editable in the editor — this is the step that actually makes the claim true; do this refactor even though it touches "old" code, since proving scalability requires the mechanism to genuinely be data-driven, not just having gotten lucky with band 0-4 being hardcoded conveniently.
2. Greybox 5 new bands (5-9) with a distinctly different geometric theme/mood from Level 1 (different color lighting is enough to prove the point even at greybox fidelity — full art pass on Level 2 is explicitly out of Month-1 scope, documented in Future Roadmap).
3. Add the corresponding `FAltitudeBand` and `FGuardianSpawnConfig` entries via the new data asset, with zero changes to `AltitudeStreamingManager.cpp`/`MonolithActor.cpp` beyond having made them data-driven in step 1.
4. Full playtest ascending from band 4 into band 5, confirming the transition between Level 1 and Level 2 is seamless (streaming, guardian spawning, checkpoint indexing all continue correctly across the level boundary).
5. Commit.

**Validation Checklist:**
- [ ] Bands 5-9 function correctly with zero new C++ logic beyond the data-asset refactor
- [ ] Transition from band 4 to band 5 (Level 1 → Level 2) is seamless in-game
- [ ] Distinctly different visual mood confirmed between the two levels even at greybox fidelity

**Testing Checklist:**
- [ ] Full ascent playtest from band 0 through band 9, confirming continuous correct behavior across both levels

**Git Commit Message:**
```
[P3.12] Add greybox Level 2 via data-driven band configuration, proving scalability
```

**Documentation Updates:** `README.md` Future Roadmap section — note that Levels 3+ are the same data-only process, ready for post-Month-1 content production.

**Common Mistakes:**
- Skipping the data-asset refactor and just hardcoding bands 5-9 the same way 0-4 were done — this would fail to actually demonstrate the scalability claim, merely repeating the same non-scalable pattern twice.

**Next Prompt Dependency:** P3.13 adds the seasonal reset logic now that there's meaningfully more than one level to reset.

---

## P3.13 — Seasonal Reset Flow

**Prerequisites / State Check:** `P3.12` complete.

**Objective:** Implement the season lifecycle: an admin/scheduled action that closes the current season (marks it inactive in `SEASONS`), opens a new one, and correctly resets what should reset (role choice, per-season share/checkpoint progress) while preserving what shouldn't (the `PLAYERS` account row itself, historical `SEASON_RANKINGS` for past seasons).

**Expected Output:** A backend endpoint/scheduled job that transitions seasons, verified by confirming a player must re-select a role and re-earn checkpoints in the new season, while their historical rank from the previous season remains queryable.

**Files to Modify:**
- `Backend/MonolithV.Api/Controllers/SeasonsController.cs` (new — `POST /seasons/current/end`, `POST /seasons` to start a new one)
- `Backend/MonolithV.Data/SeasonRepository.cs` (new)

**Implementation Steps:**
1. Implement `POST /seasons/current/end`: sets `IS_ACTIVE = 0` and `ENDS_AT = SYSTIMESTAMP` on the current active season row, inside a transaction.
2. Implement `POST /seasons`: creates a new `SEASONS` row with the next `SEASON_NUMBER`, `IS_ACTIVE = 1`, `STARTED_AT = SYSTIMESTAMP` — reject if another season is already active (must end the current one first).
3. Confirm (by design, not new code) that `PLAYER_SEASON_ROLES`, `SHARE_EVENTS`, `CHECKPOINT_PROGRESS`, and `SEASON_RANKINGS` are all keyed by `SEASON_ID`, meaning a new season naturally starts with zero rows for every player — no explicit "reset" deletion logic is needed or wanted (historical data for past seasons must remain queryable for a leaderboard-of-past-seasons feature, even if Month 1 doesn't build UI for it).
4. For Month 1, trigger season transitions manually via this endpoint (a real scheduled cron-style automated season-length timer is a reasonable Future Roadmap item, not required for the vertical slice) — document this as an intentional scope boundary.
5. Test: end the current season, start a new one, confirm a previously-role-selected player is prompted to choose again (P3.1 flow naturally handles this since `PLAYER_SEASON_ROLES` has no row for the new `SEASON_ID`), and confirm the old season's `SEASON_RANKINGS` rows are still queryable.
6. Commit.

**Validation Checklist:**
- [ ] Season end/start endpoints work correctly, reject double-active-season states
- [ ] Player correctly re-prompted for role in new season
- [ ] Past season's ranking data remains intact and queryable after transition

**Testing Checklist:**
- [ ] Full manual season-transition test with at least one player who had progress in the old season

**Git Commit Message:**
```
[P3.13] Implement seasonal reset flow via season lifecycle endpoints
```

**Documentation Updates:** `Docs/SRS.md` — mark seasonal reset FR complete, noting manual-trigger scope boundary for Month 1.

**Common Mistakes:**
- Writing explicit "DELETE FROM ... WHERE SEASON_ID = old" reset logic — unnecessary and destructive; the season-keyed schema design from P1.7 already makes new seasons start clean without deleting historical data.

**Next Prompt Dependency:** P3.14 builds the leaderboard UI reading this same season-keyed ranking data.

---

## P3.14 — Basic Leaderboard UI

**Prerequisites / State Check:** `P3.13` complete. `SEASON_RANKINGS` populated from P2.8 testing.

**Objective:** A simple in-game UMG screen showing the current season's `SEASON_RANKINGS`, reachable from a menu, reading directly from the backend.

**Expected Output:** A leaderboard widget listing rank + player display name for the active season, refreshed on open.

**Files to Modify:**
- `Backend/MonolithV.Api/Controllers/SeasonsController.cs` (add `GET /seasons/current/rankings`, joining `SEASON_RANKINGS` to `PLAYERS` for display names)
- `Game/Content/UI/WBP_Leaderboard` (new)
- `Game/Source/MonolithV/UI/LeaderboardWidget.h/.cpp` (new)

**Implementation Steps:**
1. Implement the `GET /seasons/current/rankings` endpoint (async, cached in Redis with a short TTL since this is a read-heavy, eventually-consistent-is-fine endpoint — good second real use of the P1.9 cache pattern).
2. Implement `ULeaderboardWidget`, calling this endpoint via `UBackendApiClient` on widget open, populating a `UListView` with rank + display name.
3. Add a menu entry/keybind to open the leaderboard from the HUD (P3.10).
4. Test with the multi-player ranking data already produced during P2.8/P2.12 testing (or generate fresh test claims), confirm correct ordering and display names.
5. Commit.

**Validation Checklist:**
- [ ] Leaderboard displays correct rank order matching `SEASON_RANKINGS`
- [ ] Redis caching applied and TTL behavior confirmed (same pattern/verification as P1.9)

**Testing Checklist:**
- [ ] Open leaderboard with 3+ ranked test players, confirm correct order and names

**Git Commit Message:**
```
[P3.14] Add leaderboard UI reading cached season rankings
```

**Documentation Updates:** `Docs/UserManual.md` — "Viewing the Leaderboard" section.

**Common Mistakes:**
- Forgetting the join to `PLAYERS` for display names, shipping a leaderboard of raw player IDs — not presentation-worthy, fix before considering this "done."

**Next Prompt Dependency:** P3.15 is the full vertical-slice playtesting/validation pass closing out Phase 3.

---

## P3.15 — Full Vertical-Slice Playtest & Validation

**Prerequisites / State Check:** P3.1–P3.14 complete.

**Objective:** One deliberate, honest, start-to-finish playtest of the entire vertical slice as a real player would experience it — login through EOS, choose a role, climb both levels, fight guardians, share the item, pass the gate, reach the top, see your rank, die and respawn at least once, view the leaderboard — recording every rough edge found.

**Expected Output:** A completed playtest log in `Docs/Testing/TestPlan.md` ("Phase 3 Full Playtest"), and a prioritized bug/polish list feeding directly into Phase 4.

**Files to Modify:**
- `Docs/Testing/TestPlan.md`
- `PROJECT_STATE.md` (Known Issues section populated with anything found but deferred)

**Implementation Steps:**
1. Recruit a second person if at all possible (even a non-technical friend) to be the second player for this test — a solo dev testing "both sides" of a 2-player interaction sequentially is a much weaker test than genuine simultaneous play.
2. Play the entire loop start to finish as described in the Objective, on the packaged dedicated server (not PIE), across both real network machines if possible (not just localhost).
3. Record every issue found, severity-tagged (blocking / significant / cosmetic), in `Docs/Testing/TestPlan.md`.
4. Fix anything blocking before proceeding to Phase 4; log everything else (significant/cosmetic) in `PROJECT_STATE.md`'s Known Issues for prioritization in Phase 4's bug bash (P4.9).
5. Commit.

**Validation Checklist:**
- [ ] Full loop completed successfully by two real players end-to-end at least once
- [ ] All blocking issues found are fixed before this prompt is marked complete
- [ ] All non-blocking issues are logged, not silently dropped

**Testing Checklist:**
- [ ] This prompt's entire implementation IS the testing checklist for Phase 3

**Git Commit Message:**
```
[P3.15] Complete full vertical-slice playtest and validation pass
```

**Documentation Updates:** `Docs/Testing/TestPlan.md` — "Phase 3 Full Playtest" section complete.

**Common Mistakes:**
- Testing solo by alt-tabbing between two client windows and calling it "multiplayer tested" — this misses real-world latency/timing issues that only show up between two genuinely separate machines/players; do at least one real two-machine test before calling Phase 3 done.

**Next Prompt Dependency:** This is the last prompt of Phase 3. Update `PROJECT_STATE.md`: `Last Completed Prompt ID: P3.15`, `Current Phase: Phase 3 complete → Phase 4`, `Next Prompt To Run: P4.1`. Tag `phase-3-complete`, merge `develop` → `main`. This is your biggest Twitter/X moment yet — a real playable, demoable game loop. Consider a longer-form post/thread with a full playthrough clip here specifically, even if your normal cadence is shorter updates.
