# Phase 4 — Optimization & Testing

Goal of this phase: take the working-but-unproven vertical slice from Phase 3 and produce real evidence — profiled performance numbers, bandwidth measurements, memory soak-test results, hardened server-side validation, and automated tests — that back up every claim in your SRS/abstract. Nothing new gets built here; everything gets measured, hardened, and fixed.

---

## P4.1 — Performance Profiling Pass (Client & Server)

**Prerequisites / State Check:** `PROJECT_STATE.md` → `Last Completed Prompt ID: P3.15`.

**Objective:** Capture baseline performance data for both the client and the dedicated server using Unreal Insights and in-engine `stat` commands, across both levels and under the Phase 3 full-loop scenario, establishing the numbers everything else in this phase either confirms or fixes.

**Expected Output:** A recorded Unreal Insights trace (client and server, separately) covering a full playthrough segment, plus a written summary of frame time, game thread time, and any obvious hotspots, in `Docs/Testing/TestPlan.md`.

**Files to Modify:**
- `Docs/Testing/TestPlan.md`
- `Docs/Testing/Traces/` (new folder — store or link to trace files; large traces may need external storage/Git LFS, document the choice)

**Implementation Steps:**
1. Launch the dedicated server and a client with `-trace=default,screenshot,stat` (or via Unreal Insights' session browser) to capture both processes simultaneously during a representative test session (ascend both levels, fight several guardians, share an item, claim checkpoints).
2. Open the traces in Unreal Insights; record: average/95th-percentile frame time (client), average/95th-percentile game thread tick time (server), and the top 5 most expensive functions/systems by inclusive time on each.
3. Run `stat unit`, `stat game`, `stat gpu` live during a manual playtest as a simpler cross-check against the Insights data.
4. Write findings into `Docs/Testing/TestPlan.md` under "Phase 4 — Baseline Performance", explicitly flagging anything above target thresholds you set (e.g. client frame time budget for a smooth 60fps display refresh even though server ticks at 30Hz; server game-thread tick comfortably under the 33ms/tick budget implied by 30Hz).
5. Commit.

**Validation Checklist:**
- [ ] Both client and server traces captured for a representative session
- [ ] Frame time / tick time numbers recorded with actual figures, not estimates
- [ ] Top hotspots identified and listed, even if not yet fixed (fixes happen in later prompts if warranted)

**Testing Checklist:**
- [ ] Confirm the captured session actually exercises every major system (movement, combat, streaming, guardian AI, backend calls) so the trace is representative, not just idle standing

**Git Commit Message:**
```
[P4.1] Capture baseline performance profiling data (client + server)
```

**Documentation Updates:** `Docs/Testing/TestPlan.md` — "Baseline Performance" section with real numbers.

**Common Mistakes:**
- Profiling only the client and assuming server performance is "probably fine" — the server is the authoritative simulation for every connected player; its game-thread time is the actual scalability ceiling and must be measured directly.
- Profiling an idle/empty scene — always profile against the busiest realistic scenario (multiple guardians + players + streaming transition happening together).

**Next Prompt Dependency:** P4.2 addresses network bandwidth specifically, informed by whatever this profiling pass surfaced about replication cost.

---

## P4.2 — Network Bandwidth & Interest Management

**Prerequisites / State Check:** `P4.1` complete.

**Objective:** Measure actual per-client bandwidth usage (`stat net`) during the busiest realistic scenario (multiple guardians + a second player, all in the same band), then apply basic interest-management tuning — `NetCullDistanceSquared` on guardians/props, relevant-actor culling for distant bands — so bandwidth scales reasonably as guardian count grows, rather than blindly replicating everything to everyone regardless of relevance.

**Expected Output:** A documented before/after bandwidth comparison showing the effect of tuned `NetCullDistanceSquared` and relevancy settings, with numbers recorded, not just "it should be better."

**Files to Modify:**
- `Game/Source/MonolithV/AI/GuardianCharacter.cpp` (set `NetCullDistanceSquared` appropriately for guardian relevance radius)
- `Game/Source/MonolithV/World/MonolithActor.cpp` (ensure actors in unloaded/irrelevant bands are not replicated to players in unrelated bands — verify `IsNetRelevantFor` behavior is correct, since Unreal's default distance-based relevancy should already mostly handle this, but must be verified, not assumed)
- `Docs/Testing/TestPlan.md`

**Implementation Steps:**
1. Run `stat net` on a client while multiple guardians are active in the same and adjacent bands, record `Net Actor Count`, `In/Out Bandwidth`.
2. Set `NetCullDistanceSquared` on `AGuardianCharacter` to a sensible value matching actual gameplay-relevant sight/engagement range (guardians far beyond player interaction range shouldn't consume replication bandwidth).
3. Verify (via the P2.5 streaming manager) that actors in bands the player isn't near are already excluded from replication as a side effect of not being loaded/spawned in the first place — this is the deliberate design synergy between pillar 1 (networking) and pillar 2 (streaming): unloaded bands cost zero replication bandwidth, not just zero memory, because their actors don't exist yet.
4. Re-run the same busy-scenario test, record the new `stat net` numbers, compare directly against the P4.1/step-1 baseline.
5. Document the before/after numbers and the specific settings changed in `Docs/Testing/TestPlan.md`.
6. Commit.

**Validation Checklist:**
- [ ] `NetCullDistanceSquared` tuned and confirmed to reduce irrelevant-actor replication in testing
- [ ] Confirmed (not assumed) that unloaded-band actors do not consume replication bandwidth
- [ ] Before/after bandwidth numbers recorded

**Testing Checklist:**
- [ ] `stat net` comparison run under identical scenarios before and after tuning, numbers recorded in the test plan

**Git Commit Message:**
```
[P4.2] Tune interest management and measure bandwidth impact
```

**Documentation Updates:** `Docs/Testing/TestPlan.md` — bandwidth before/after section; this directly backs the "bandwidth optimization" and "interest management" claims in your SRS.

**Common Mistakes:**
- Asserting the streaming-manager/relevancy synergy works without actually measuring it — verify with real `stat net` numbers; it's an easy claim to get subtly wrong (e.g. if actors aren't properly destroyed on band unload, they'd still replicate even though invisible).

**Next Prompt Dependency:** P4.3 does the equivalent rigor pass for memory, extending P2.5's basic check into a real soak test.

---

## P4.3 — Memory & Streaming Soak Test

**Prerequisites / State Check:** `P4.2` complete.

**Objective:** Extend the single before/after memory check from P2.5 into a genuine soak test: repeatedly cycling a player up and down through all bands/levels many times, watching for memory creep (a sign of leaked actors/assets not being properly released on unload) rather than just confirming one unload event frees memory once.

**Expected Output:** A memory-over-time graph/log (`stat memory` sampled at intervals, or an Unreal Insights memory trace) across at least 20 full band-cycle repetitions, with either a flat/stable memory profile confirmed, or a found-and-fixed leak documented.

**Files to Modify:**
- Any file where a leak is found and fixed (likely candidates: guardian spawner not fully destroying actors on band unload, or a component holding a stale reference preventing garbage collection)
- `Docs/Testing/TestPlan.md`

**Implementation Steps:**
1. Write a simple test harness (can be a debug console command or a temporary automated test script) that teleports a test player up through all 10 bands and back down, repeated 20+ times, sampling `stat memory` (or logging `FPlatformMemory::GetStats()` programmatically) at each full cycle.
2. Plot or tabulate memory usage per cycle; a healthy system shows memory returning to roughly the same baseline after each full down-cycle, not a steady upward trend.
3. If a leak is found, use Unreal's memory profiling tools (`memreport`, or Insights' memory insights if available for your engine version) to identify what's not being released — the most likely culprits given this project's design are: guardians not fully destroyed (only hidden) on band unload from P3.8, or a lingering hard reference somewhere (e.g. a delegate binding on a destroyed actor never unbound) keeping the unloaded level's package resident.
4. Fix any found leak, re-run the soak test to confirm the fix, and record final numbers.
5. Commit.

**Validation Checklist:**
- [ ] 20+ cycle soak test completed with numbers recorded
- [ ] Memory profile is stable (no meaningful upward trend) after the test/fix
- [ ] Any leak found is fixed and the fix is verified by re-running the soak test

**Testing Checklist:**
- [ ] Soak test re-run after any fix to confirm resolution, not just "should be fixed now"

**Git Commit Message:**
```
[P4.3] Complete memory soak test and fix streaming-related leaks
```

**Documentation Updates:** `Docs/Testing/TestPlan.md` — "Memory Soak Test" section with the cycle-by-cycle data; this is strong, concrete evidence for your pillar 2 claims at your defense.

**Common Mistakes:**
- Running only 2-3 cycles and calling it a soak test — leaks are often only visible as a trend over many repetitions; a single before/after check (like P2.5's) is a smoke test, not a soak test, and this prompt exists specifically to go further.

**Next Prompt Dependency:** P4.4 revisits prediction/lag-compensation now under more realistic combat load.

---

## P4.4 — Lag Compensation Review for Hit Registration

**Prerequisites / State Check:** `P4.3` complete.

**Objective:** Honestly evaluate hit-registration fairness under latency for the P3.5 hit-scan weapon: at 30Hz server tick with client-side prediction (P2.3) but no server-side rewind/lag-compensation for hit detection, a fast-moving target can legitimately be "not where the shooter saw them" from the server's perspective. Decide, document, and if time allows, implement a bounded fix — or explicitly scope it out with a clear justification, rather than leaving it as an unexamined gap.

**Expected Output:** A written analysis in `Docs/Testing/TestPlan.md` of observed hit-registration accuracy at simulated latency (using the P2.3 net-emulation technique), and either (a) a simple, bounded server-side rewind implementation for hit-scan checks, or (b) an explicit, justified decision to accept the current behavior for Month 1 with the tradeoff documented for your report/defense.

**Files to Modify:**
- `Game/Source/MonolithV/Combat/GA_FireWeapon.cpp` (if implementing rewind: sample target position history)
- `Docs/Testing/TestPlan.md`

**Implementation Steps:**
1. Using `Net PktLag=100` (simulating realistic internet latency), test hit-scan accuracy against a moving guardian/player target, recording how often visually-correct shots fail to register due to server-side position lag.
2. If the miss rate is high enough to meaningfully hurt gameplay feel (your judgment call, but document the threshold you used), implement a minimal server-side rewind: maintain a short ring buffer (e.g. last 1 second at 30Hz = 30 samples) of each potential target's position, and when validating a hit-scan trace, check against the target's position at `(server time - reported client RTT/2)` rather than only its current position.
3. If choosing NOT to implement rewind for Month 1 (a legitimate choice given the scope), write the justification explicitly: e.g. "at the tested latency range and target speeds, miss rate was Z%, deemed acceptable for a vertical-slice demo; full lag compensation is a documented Future Roadmap item requiring more testing time than available."
4. Whichever path taken, record the actual test numbers (hit rate before/after, or hit rate accepted as-is) in `Docs/Testing/TestPlan.md`.
5. Commit.

**Validation Checklist:**
- [ ] Hit-registration behavior under simulated latency is measured, not assumed
- [ ] A clear decision (implement or explicitly defer) is made and justified with real numbers
- [ ] If implemented, rewind is bounded (fixed buffer size) and doesn't allow exploitable "shoot into the past" abuse beyond the buffer window

**Testing Checklist:**
- [ ] Latency-emulated hit-registration test performed and recorded, whichever path was chosen

**Git Commit Message:**
```
[P4.4] Evaluate and address hit-registration lag compensation
```

**Documentation Updates:** `Docs/SRS.md` non-functional requirements — record the actual decision and its justification; this is exactly the kind of honest engineering tradeoff a strong final-year report should surface rather than hide.

**Common Mistakes:**
- Silently ignoring this issue without measurement or documentation — an evaluator asking "how do you handle lag compensation" deserves a measured, reasoned answer either way, not a project that never considered the question.

**Next Prompt Dependency:** P4.5 tests overall system behavior under concurrent load beyond the 2-4 player manual tests done so far.

---

## P4.5 — Load / Soak Test at Target Concurrency

**Prerequisites / State Check:** `P4.4` complete.

**Objective:** Validate the system holds up with more simultaneous load than manual 2-client testing can easily provide — simulate additional concurrent backend traffic (share/checkpoint requests) and, if feasible, additional bot/AI-controlled client connections, to stress the atomic transaction endpoints (P2.7/P2.8) and the dedicated server's tick budget under closer-to-realistic concurrent conditions.

**Expected Output:** A load-test report showing the backend correctly handling a burst of concurrent requests (extend the P2.7/P2.8 concurrency unit tests into a larger-scale load test, e.g. 100+ concurrent requests via a simple load-testing tool), and, if bot clients are added, server tick-time behavior under 4+ simultaneous connections instead of just 2.

**Files to Modify:**
- `Backend/MonolithV.Tests/LoadTests/` (new — a simple load test project or script, e.g. using `k6`, `NBomber`, or a hand-rolled `Task.WhenAll` burst test extending P2.7/P2.8's pattern)
- `Docs/Testing/TestPlan.md`

**Implementation Steps:**
1. Extend the existing concurrency tests (P2.7/P2.8) to a genuine load test: fire a large burst (100+) of realistic mixed requests (some share-events, some checkpoint claims, some player/leaderboard reads) concurrently against a locally-running backend instance, recording response time distribution and error rate.
2. Confirm zero data-integrity violations under this load (spot-check `SHARE_EVENTS`/`SEASON_RANKINGS` row counts match expectations, same correctness check as the earlier concurrency tests, just at higher volume).
3. If your timeline allows: connect 4 real or bot-controlled clients to the dedicated server simultaneously (Unreal supports launching multiple `-game` instances locally for this), and measure server game-thread tick time (`stat game`) under this load, comparing against the P4.1 baseline (2-client) numbers.
4. Record all findings, including any degradation observed and whether it's within acceptable bounds for the project's target concurrency (2-4 players, per your original scope decision).
5. Commit.

**Validation Checklist:**
- [ ] Backend load test completed with recorded response-time/error-rate numbers
- [ ] Data integrity confirmed correct at higher request volume (no duplicate/lost rows)
- [ ] Server tick time measured at target concurrency (4 clients), compared against baseline

**Testing Checklist:**
- [ ] Load test numbers recorded in `Docs/Testing/TestPlan.md`, including any errors encountered and how they were resolved

**Git Commit Message:**
```
[P4.5] Complete load/soak testing at target concurrency
```

**Documentation Updates:** `Docs/Testing/TestPlan.md` — "Load Testing" section; strong evidence for the SRS performance non-functional requirements.

**Common Mistakes:**
- Load-testing only the backend and never re-validating the actual Unreal dedicated server under multiple real connections — both matter; the backend proves pillar 3/4's claims, the multi-client server test proves pillar 1's claims.

**Next Prompt Dependency:** P4.6 does a second, deeper server-side validation audit now that Phase 3 added many more client-triggered actions since P2.11's first pass.

---

## P4.6 — Server-Side Validation Hardening (Round 2)

**Prerequisites / State Check:** `P4.5` complete. `Docs/Architecture/AntiCheatAudit.md` from P2.11 exists but predates Phase 3's weapon/dash/jetpack/guardian systems.

**Objective:** Repeat and extend the P2.11 audit methodology across everything Phase 3 added — weapon firing, dash-attack, jetpack, guardian damage — since each of these is a new client-triggerable-intent path that must be independently verified as server-validated, not assumed safe by association with the already-audited systems.

**Expected Output:** An updated `AntiCheatAudit.md` covering every entry point in the full game, with at least one more concrete hardening fix applied (e.g. a server-side sanity check that weapon damage-per-second cannot exceed the theoretical max given `FireRate`/`Damage`, catching a tampered fire-rate client).

**Files to Modify:**
- `Docs/Architecture/AntiCheatAudit.md`
- `Game/Source/MonolithV/Combat/GA_FireWeapon.cpp` (add a server-side rate sanity check independent of the GAS cooldown gate, as defense-in-depth)

**Implementation Steps:**
1. List every Phase 3 client-triggerable action (fire weapon, dash-attack, jetpack activation, share-interact, gate-pass attempt) alongside its existing server-side validation, same format as P2.11.
2. Add one concrete defense-in-depth check: track each player's actual observed damage-dealt-per-second server-side (a rolling counter), and log (at minimum) or reject (if you choose to implement enforcement) any player exceeding the theoretical max given their weapon's configured `FireRate`/`Damage` — this catches a tampered/scripted fire-rate bypass even if it somehow got past the GAS cooldown gate.
3. Attempt to manually "cheat" each Phase 3 system the same way P2.11 did for Phase 2's systems (fire faster than cooldown allows via input spam, trigger dash-attack from an impossible distance/state, claim jetpack fuel that shouldn't exist) and confirm each is correctly rejected.
4. Commit.

**Validation Checklist:**
- [ ] `AntiCheatAudit.md` updated to cover all Phase 3 systems
- [ ] Damage-rate sanity check implemented and tested against a simulated fire-rate-spam attempt
- [ ] Manual cheat-attempt pass repeated for every Phase 3 system, all correctly rejected

**Testing Checklist:**
- [ ] Each Phase 3 entry point's "cheat attempt" test explicitly performed and recorded pass/fail

**Git Commit Message:**
```
[P4.6] Extend anti-cheat audit and hardening to Phase 3 systems
```

**Documentation Updates:** `Docs/Architecture/AntiCheatAudit.md` — full, current audit covering the entire game.

**Common Mistakes:**
- Assuming "it uses GAS so it's automatically safe" — GAS provides the mechanism for validation (cooldowns, costs, server-authoritative execution) but each ability's specific game-logic invariants (distance checks, rate sanity) still need to be deliberately written, as proven necessary in P2.11 and again here.

**Next Prompt Dependency:** P4.7 backs all of this with actual automated tests rather than only manual verification.

---

## P4.7 — Automated Unit Tests for Transaction Logic

**Prerequisites / State Check:** `P4.6` complete. `MonolithV.Tests` project exists since P1.5, with concurrency tests from P2.7/P2.8.

**Objective:** Given the "minimal testing" scope decision from Phase 1, focus limited test-writing time on exactly the highest-value target: the transaction/concurrency logic (share-events, checkpoint claims, season lifecycle) where correctness bugs are subtle and hard to catch by manual testing alone — not broad coverage everywhere.

**Expected Output:** An expanded `MonolithV.Tests` suite covering: role-mismatch rejection, idempotent share/claim behavior, rank-assignment correctness under concurrency (already from P2.7/P2.8, confirmed still passing), season-transition edge cases (double-active rejection), and leaderboard query correctness.

**Files to Modify:**
- `Backend/MonolithV.Tests/ShareEventTests.cs` (expand beyond just concurrency — add role-mismatch, already-shared cases)
- `Backend/MonolithV.Tests/CheckpointTests.cs` (expand similarly)
- `Backend/MonolithV.Tests/SeasonLifecycleTests.cs` (new)

**Implementation Steps:**
1. Add test cases (beyond the existing concurrency tests) for: same-role share rejection, already-shared idempotent no-op, out-of-range distance rejection (if this validation lives in the backend rather than only the game client/server — confirm where this check actually lives and test it at that layer).
2. Add `SeasonLifecycleTests`: starting a season while one is already active is rejected; ending with no active season is rejected; a new season correctly isolates a player's `PLAYER_SEASON_ROLES` from the prior season's row.
3. Ensure all tests run against either a real disposable test schema on the Autonomous DB (a separate `SEASON_ID`/test-prefixed data, cleaned up after each test run) or a documented local Oracle test-container approach — do not test against production-intended data.
4. Run the full suite, confirm all pass, wire it into the CI workflow from P1.10 (already runs `dotnet test`, so this is largely automatic — just confirm the new tests are correctly discovered and run).
5. Commit.

**Validation Checklist:**
- [ ] All new test cases pass
- [ ] Tests run against isolated test data, not production rows
- [ ] CI (`P1.10`'s workflow) picks up and runs the expanded suite automatically

**Testing Checklist:**
- [ ] Full `dotnet test` run locally and via CI, all green

**Git Commit Message:**
```
[P4.7] Expand automated tests for transaction and season-lifecycle logic
```

**Documentation Updates:** `Docs/Testing/TestPlan.md` — list of automated test cases and what each guards against; `Docs/SDLC.md` — note the deliberate "minimal but high-value" testing strategy and why it was chosen (solo, 1 month, prioritizing correctness-critical logic over broad coverage).

**Common Mistakes:**
- Writing tests against the same Oracle rows used for manual demo/playtesting — a test run wiping or altering demo data right before a presentation is an entirely avoidable, embarrassing risk; keep test data clearly isolated.

**Next Prompt Dependency:** P4.8 hardens CI now that there's a meaningful test suite to protect.

---

## P4.8 — CI Hardening

**Prerequisites / State Check:** `P4.7` complete.

**Objective:** Strengthen the P1.10 CI workflow now that there's real test coverage worth protecting: ensure builds fail loudly on any regression, add basic status badges to the README for a professional public-repo appearance, and formally document (rather than silently accept) the still-manual Unreal compile-check step.

**Expected Output:** A CI workflow with clear pass/fail signal, a green build badge on `README.md`, and an explicit, documented manual pre-merge checklist for the Unreal side.

**Files to Modify:**
- `.github/workflows/ci.yml`
- `README.md` (add CI status badge)
- `.github/PULL_REQUEST_TEMPLATE.md` (confirm/expand the manual Unreal-compile checkbox from P1.10)

**Implementation Steps:**
1. Confirm the CI workflow fails the whole job (not just logs a warning) on any test failure — verify by intentionally breaking a test on a throwaway branch.
2. Add a GitHub Actions status badge markdown snippet to the top of `README.md`, pointing at the `ci.yml` workflow.
3. Review the PR template's manual Unreal-compile checkbox; make sure it's actually being honored in practice (spot-check your own recent merge history) — if it's being skipped in practice, that's a process gap worth fixing before Phase 5, not just a documentation nicety.
4. Commit.

**Validation Checklist:**
- [ ] CI fails visibly and correctly on an intentionally broken test
- [ ] Badge renders correctly on the GitHub README
- [ ] Manual Unreal-compile step is being genuinely followed, confirmed via recent PR history

**Testing Checklist:**
- [ ] Intentional break/fix cycle on a throwaway branch, confirming red-then-green CI status

**Git Commit Message:**
```
[P4.8] Harden CI status signaling and add build badge
```

**Documentation Updates:** `README.md` badge; `Docs/SDLC.md` — note final CI/testing posture as of Phase 4.

**Common Mistakes:**
- Adding a badge that doesn't actually reflect current pass/fail status (e.g. hardcoded, or pointing at the wrong workflow file) — verify it updates correctly by watching it change color after triggering a run.

**Next Prompt Dependency:** P4.9 works through the accumulated known-issues list now that testing/hardening infrastructure is solid.

---

## P4.9 — Prioritized Bug Bash

**Prerequisites / State Check:** `P4.8` complete. `PROJECT_STATE.md` Known Issues section populated from P3.15 and any issues found during P4.1–P4.8.

**Objective:** Systematically work through every known issue logged so far, fixing everything blocking or significant, making a deliberate documented call on anything cosmetic that gets deferred to Future Roadmap instead.

**Expected Output:** An empty (or fully-triaged-and-justified) `PROJECT_STATE.md` Known Issues section, with every fix verified by re-running the relevant original test from whichever prompt first surfaced the issue.

**Files to Modify:** Varies per issue — likely touches multiple files across `Game/Source/MonolithV/` and `Backend/`.
**Files to Modify (tracking):** `PROJECT_STATE.md`, `Docs/Testing/TestPlan.md`

**Implementation Steps:**
1. List every open issue from `PROJECT_STATE.md`, tag each blocking/significant/cosmetic.
2. Fix all blocking and significant issues, one at a time, each verified against its originating test (e.g. a P3.9 respawn bug gets re-tested against P3.9's own validation checklist after the fix, not just eyeballed).
3. For any cosmetic issue deliberately deferred, move it explicitly into `README.md`'s Future Roadmap section (not just left dangling) so it's a documented decision, not a forgotten gap.
4. Update `PROJECT_STATE.md` Known Issues to reflect the new (hopefully empty, or fully-justified) state.
5. Commit (likely several commits, one per fixed issue, each with its own descriptive message prefixed `[P4.9]`).

**Validation Checklist:**
- [ ] Every blocking/significant issue fixed and re-verified against its origin test
- [ ] Every deferred cosmetic issue is explicitly documented in Future Roadmap, not silently dropped
- [ ] `PROJECT_STATE.md` Known Issues section accurately reflects final state

**Testing Checklist:**
- [ ] Each fix re-tested against its originating prompt's validation checklist, not just spot-checked informally

**Git Commit Message:**
```
[P4.9] Complete prioritized bug bash across known issues
```

**Documentation Updates:** `PROJECT_STATE.md`, `README.md` Future Roadmap (for deferred items).

**Common Mistakes:**
- Fixing a bug and moving on without re-running the original validation checklist that first defined "correct" behavior for that system — regressions are easy to reintroduce without this discipline.

**Next Prompt Dependency:** P4.10 does a focused security/secrets review before the codebase goes further toward public deployment in Phase 5.

---

## P4.10 — Security & Secrets Review

**Prerequisites / State Check:** `P4.9` complete.

**Objective:** A focused pass specifically on secrets handling and dependency hygiene before Phase 5's public cloud deployment: confirm no credentials have ever been committed (across full git history, not just the current tree), and check for any known-vulnerable NuGet/plugin dependencies.

**Expected Output:** A clean `git log -p` history with respect to secrets (or, if something was previously committed, a documented remediation — e.g. credential rotation — since removing from history alone doesn't invalidate an already-exposed secret), and a dependency audit report.

**Files to Modify:**
- `Docs/RiskAnalysis.md` (fill in real content — this was stubbed in P1.13)

**Implementation Steps:**
1. Search full git history (not just current files) for accidentally committed secrets: `git log -p -- '*.json' '*.ini'` grepped for connection strings/API keys/client secrets, and specifically check the EOS `DefaultEngine.ini` and backend `appsettings.*.json` history.
2. If anything sensitive is found in history, the correct remediation is credential rotation (generate new EOS client secret, new DB password) — not just deleting the file or force-pushing history rewrite, since a secret once exposed (even briefly, even in a private moment) should be treated as compromised.
3. Run `dotnet list package --vulnerable` (built into modern .NET SDKs) across the `Backend/` solution, address any flagged high/critical vulnerable packages by upgrading.
4. Fill in `Docs/RiskAnalysis.md` properly now: technical risks (e.g. solo-dev bus factor, Oracle Free Tier resource limits, EOS sandbox rate limits), mitigations, and residual risk acceptance for Month 1 scope.
5. Commit.

**Validation Checklist:**
- [ ] No secrets found in current tree or full git history (or, if found, rotated and documented)
- [ ] `dotnet list package --vulnerable` clean or all findings addressed
- [ ] `Docs/RiskAnalysis.md` complete with real, specific risks (not generic textbook risk-register boilerplate)

**Testing Checklist:**
- [ ] Full git history secret-scan performed and documented, not just a spot-check of current files

**Git Commit Message:**
```
[P4.10] Complete security review and finalize risk analysis
```

**Documentation Updates:** `Docs/RiskAnalysis.md` — complete.

**Common Mistakes:**
- Treating "I added it to .gitignore" as equivalent to "it was never exposed" — .gitignore only prevents future commits; anything already committed (even in an old, since-amended commit) remains in history unless explicitly purged, and even then should be considered exposed if the repo was ever pushed publicly.

**Next Prompt Dependency:** P4.11 compiles all of Phase 4's measurements into a final report.

---

## P4.11 — Performance & Testing Report Compilation

**Prerequisites / State Check:** `P4.10` complete.

**Objective:** Consolidate every number gathered across P4.1–P4.10 (frame time, bandwidth, memory soak results, load test results, lag-compensation findings) into one coherent `Docs/Testing/PerformanceReport.md` — the single document you'd hand an evaluator asking "prove your optimization claims."

**Expected Output:** A clean, well-organized report with tables/numbers pulled from the scattered `TestPlan.md` entries across this phase, plus a short "what we'd do differently at larger scale" section (tying directly into Future Roadmap).

**Files to Modify:**
- `Docs/Testing/PerformanceReport.md` (new)

**Implementation Steps:**
1. Pull together, in one document, with clear headers per topic: baseline performance (P4.1), bandwidth before/after (P4.2), memory soak results (P4.3), lag-compensation findings (P4.4), load test results (P4.5).
2. Add a short interpretive paragraph per section — not just raw numbers, but what they mean and whether targets were met.
3. Add a closing "Scaling Beyond Month 1" section explicitly connecting these findings to specific Future Roadmap items (e.g. "bandwidth scales linearly with guardian count per band; supporting larger concurrent player counts would require the Replication Graph plugin instead of default relevancy, documented as a Future Roadmap item").
4. Commit.

**Validation Checklist:**
- [ ] Every Phase 4 measurement is represented in the final report
- [ ] Report is readable standalone by an evaluator with no other context

**Testing Checklist:**
- [ ] Cross-check every number in the report against its original source entry in `TestPlan.md` for accuracy (no transcription errors)

**Git Commit Message:**
```
[P4.11] Compile final performance and testing report
```

**Documentation Updates:** `Docs/Testing/PerformanceReport.md` — complete; link it from `README.md` and `Docs/SRS.md`.

**Common Mistakes:**
- Cherry-picking only the flattering numbers — an honest report that shows both what worked and what has known limits (with a clear roadmap for the limits) is more credible in an academic defense than one that suspiciously shows no weaknesses at all.

**Next Prompt Dependency:** P4.12 is the final regression pass closing out the phase.

---

## P4.12 — Final Regression Pass & Phase Close-Out

**Prerequisites / State Check:** P4.1–P4.11 complete.

**Objective:** One last full playthrough (same scope as P3.15's playtest) confirming that all of Phase 4's hardening/fixes/optimizations didn't regress any Phase 3 functionality, closing this phase with the same rigor P3.15 opened it.

**Expected Output:** A clean final playthrough with no blocking issues, `PROJECT_STATE.md` fully updated for Phase 5.

**Files to Modify:**
- `Docs/Testing/TestPlan.md`
- `PROJECT_STATE.md`

**Implementation Steps:**
1. Repeat the full P3.15 playtest scenario (login → role select → climb both levels → guardians → share → gate → checkpoints → death/respawn → leaderboard) on the current, fully-hardened build.
2. Confirm every system still functions correctly after all Phase 4 changes; fix immediately if anything regressed.
3. Record the final pass in `Docs/Testing/TestPlan.md`.
4. Commit.

**Validation Checklist:**
- [ ] Full playthrough completed with zero blocking issues on the final Phase 4 build

**Testing Checklist:**
- [ ] This prompt's entire implementation IS the testing checklist

**Git Commit Message:**
```
[P4.12] Complete final Phase 4 regression pass
```

**Documentation Updates:** `Docs/Testing/TestPlan.md` — final Phase 4 entry.

**Common Mistakes:**
- Skipping this because "each individual fix was already tested" — the point is catching interactions between fixes, exactly like P2.12's integration test caught issues individual system tests couldn't.

**Next Prompt Dependency:** This is the last prompt of Phase 4. Update `PROJECT_STATE.md`: `Last Completed Prompt ID: P4.12`, `Current Phase: Phase 4 complete → Phase 5`, `Next Prompt To Run: P5.1`. Tag `phase-4-complete`, merge `develop` → `main`. Good Twitter/X moment: share one concrete number from the performance report (e.g. bandwidth-per-player or the memory-soak graph) — technical credibility content resonates differently than gameplay clips and diversifies your dev-log audience.
