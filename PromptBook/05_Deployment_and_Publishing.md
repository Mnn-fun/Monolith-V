# Phase 5 — Deployment & Publishing

Goal of this phase: move from "works on my local VM" to "actually deployed on Oracle Cloud, reachable over the real internet, documented well enough that a stranger can install and play it, and presentable in front of evaluators." This is the phase that turns a finished build into a finished *submission*.

---

## P5.1 — Deploy Backend + Redis to Production (Oracle Cloud VM)

**Prerequisites / State Check:** `PROJECT_STATE.md` → `Last Completed Prompt ID: P4.12`. Oracle Cloud VM from P1.6 running and reachable via SSH.

**Objective:** Containerize and deploy the ASP.NET backend + Redis to the Oracle Cloud production VM via Docker Compose, replacing the local-VM-only deployment used throughout development.

**Expected Output:** `MonolithV.Api` and Redis both running as Docker containers on the Oracle Cloud VM, `GET /health` reachable from your local machine over the public internet at the VM's public IP.

**Files to Modify:**
- `Backend/MonolithV.Api/Dockerfile` (new)
- `Infra/docker-compose.prod.yml` (new — production variant: real Oracle connection string via environment variable, Redis with a persistent volume)
- `Infra/cloud/deploy-server.sh` (implement fully — was a placeholder/reference in P1.6)

**Implementation Steps:**
1. Write a multi-stage `Dockerfile` for `MonolithV.Api` (SDK image to build/publish, runtime image to run) — confirm the Oracle ODP.NET Core native dependencies are present in the runtime image (may require the `mcr.microsoft.com/dotnet/aspnet` base plus any Oracle client libraries the managed driver needs; ODP.NET Core's fully-managed mode should avoid needing Oracle Instant Client, but verify this explicitly by testing the container).
2. Write `docker-compose.prod.yml`: `api` service built from the Dockerfile, `redis` service (same image as dev, now with a named volume for persistence), both on a shared Docker network, `api`'s Oracle connection string and Redis host supplied via environment variables (not baked into the image).
3. `scp` the compose file (and Dockerfile context, or build+push to a registry — for a solo Month-1 project, building directly on the VM via `scp` + `docker compose build` is simpler than standing up a registry) to the Oracle Cloud VM.
4. On the VM, set the real production environment variables (Oracle Autonomous DB connection string using the VM-side copy of the wallet — copy the wallet to the VM securely via `scp`, never via git), `docker compose -f docker-compose.prod.yml up -d --build`.
5. From your local machine, `curl http://<vm-public-ip>:5000/health` (adjust for whatever port you exposed), confirm success over the real public internet, not just the local network.
6. Fill in `Infra/cloud/deploy-server.sh` with the exact deployment commands used, so this is a repeatable one-command deploy for any future update.
7. Commit (Dockerfile, compose file, deploy script — never the wallet or real environment variable values).

**Validation Checklist:**
- [ ] Backend + Redis both running as containers on the Oracle Cloud VM
- [ ] `/health` reachable from an external network (not just VM-local or LAN)
- [ ] Wallet and secrets present on the VM but never committed to git

**Testing Checklist:**
- [ ] `curl` the health endpoint from a network genuinely outside your home/campus network (e.g. mobile data) to confirm true public reachability

**Git Commit Message:**
```
[P5.1] Deploy backend and Redis to Oracle Cloud production via Docker
```

**Documentation Updates:** `Docs/InstallationGuide.md` — add "Production Deployment" section.

**Common Mistakes:**
- Baking real connection strings/secrets into the Docker image — always inject via environment variables at container runtime, keeping the image itself shareable/rebuildable without leaking credentials.
- Forgetting to open the backend's port on the VM's firewall/security list (separate from the game's UDP port opened in P1.6) — both need explicit ingress rules.

**Next Prompt Dependency:** P5.2 runs the production database migration this backend now depends on.

---

## P5.2 — Production Database Migration

**Prerequisites / State Check:** `P5.1` complete.

**Objective:** Ensure the Oracle Autonomous DB used by the production backend has the current schema (all of `V1`–`V3` from P1.7/P2.7/P2.8) applied cleanly, and establish a simple migration-tracking convention so future schema changes are traceable.

**Expected Output:** All versioned schema scripts applied to the production DB in order, with a simple `SCHEMA_VERSION` tracking table confirming what's been applied.

**Files to Modify:**
- `Infra/oracle/schema/V0__schema_version_tracking.sql` (new — retroactively added as the first migration, tracks applied version numbers)
- `Infra/oracle/README.md` (document the migration process)

**Implementation Steps:**
1. Add `V0__schema_version_tracking.sql` (conceptually "run first" even though added last): a simple `SCHEMA_MIGRATIONS(VERSION NUMBER PRIMARY KEY, APPLIED_AT TIMESTAMP)` table.
2. Manually insert rows for `V1`, `V2`, `V3` (already applied during Phase 1/2 development against this same Autonomous DB instance — confirm you're deploying to the *same* DB instance used throughout development, not a fresh one, since Month 1's DB IS the production DB in this project's scope) reflecting when they were actually applied.
3. If you provisioned a genuinely separate DB instance for "production" vs. dev, re-run `V1`, `V2`, `V3` in order against it now, recording each in `SCHEMA_MIGRATIONS`.
4. Document in `Infra/oracle/README.md`: the convention that all future schema changes get a new `V<n>__description.sql` file, applied in order, tracked in `SCHEMA_MIGRATIONS`.
5. Verify via SQLcl that all expected tables exist and match the current schema exactly (`user_tables`, `user_tab_columns` spot-check).
6. Commit.

**Validation Checklist:**
- [ ] `SCHEMA_MIGRATIONS` table accurately reflects applied versions
- [ ] All expected tables/columns/constraints present and verified in the production DB
- [ ] Migration convention documented for future changes

**Testing Checklist:**
- [ ] Backend (from P5.1) successfully connects and performs a real query (e.g. `/players/{id}`) against this confirmed production schema

**Git Commit Message:**
```
[P5.2] Establish schema version tracking and confirm production migration
```

**Documentation Updates:** `Infra/oracle/README.md` — migration convention documented.

**Common Mistakes:**
- Standing up a brand-new "production" DB instance at the last minute and forgetting it has none of the season/ranking data accumulated during development testing — decide deliberately whether Month 1's single DB instance serves as both dev and demo/production (reasonable for this scope) or whether a clean split is worth the effort; document the choice either way.

**Next Prompt Dependency:** P5.3 deploys the actual game server binary against this now-confirmed production backend/DB.

---

## P5.3 — Deploy Dedicated Server to Oracle Cloud (Production)

**Prerequisites / State Check:** `P5.2` complete.

**Objective:** Repeat the P2.1 local-VM deployment process, this time targeting the Oracle Cloud production VM, pointed at the now-production backend (P5.1) instead of a local one.

**Expected Output:** `MonolithVServer` running on the Oracle Cloud VM, reachable from a client anywhere on the internet at the VM's public IP on the reserved UDP port, correctly calling the production backend for all share/checkpoint/season logic.

**Files to Modify:**
- `Game/Config/DefaultEngine.ini` or a runtime config override (backend base URL must point at the production VM's backend address, not `localhost`)
- `Infra/cloud/deploy-server.sh` (extend to cover the game server deployment alongside the backend)

**Implementation Steps:**
1. Confirm `UBackendApiClient`'s base URL (from P2.9) is configurable (via `DefaultEngine.ini` or a command-line-overridable config value) rather than hardcoded to `localhost` — fix now if this was hardcoded during development.
2. Re-run the P2.1 `RunUAT BuildCookRun` packaging step for the `MonolithVServer` target, `scp` the build to the Oracle Cloud VM.
3. Launch with the production backend URL configured, on the reserved UDP port (`7777` or whichever you chose), confirm the firewall/security-list rule from P1.6 is correctly allowing this traffic.
4. Test: connect a client from a genuinely external network (not the same LAN as the VM) via direct IP, confirm movement/combat/streaming all function, and confirm a share-event/checkpoint-claim correctly reaches the now-production backend/DB (spot-check via SQLcl).
5. Set up the server to actually stay running (e.g. a `systemd` service unit, or at minimum a `tmux`/`screen` session with auto-restart-on-crash via a simple wrapper script) rather than a foreground terminal session that dies when you disconnect SSH.
6. Commit the deployment script and systemd unit file (if used).

**Validation Checklist:**
- [ ] Server runs on Oracle Cloud VM, survives SSH disconnect (systemd or equivalent)
- [ ] Reachable from a genuinely external client
- [ ] Correctly calling production backend (verified via a real share/checkpoint test hitting production DB)

**Testing Checklist:**
- [ ] Full playtest loop performed against the production deployment, not just a connectivity check

**Git Commit Message:**
```
[P5.3] Deploy dedicated server to Oracle Cloud production
```

**Documentation Updates:** `Docs/InstallationGuide.md` — "Connecting to the Production Server" section with the actual connect command/IP (or document that IP is shared privately with evaluators rather than publicly in the repo, if you'd prefer not to expose a public game server address in a public README — reasonable choice, document which you picked).

**Common Mistakes:**
- Leaving the backend URL hardcoded to `localhost` from development — this is the single most common "worked on my machine, broken in production" bug class; explicitly test the client against the real production backend address before considering this done.
- Running the server in a plain SSH-attached terminal — it will die the moment you disconnect; use `systemd`, `tmux`, or `nohup` with a restart wrapper.

**Next Prompt Dependency:** P5.4 sets up production-appropriate EOS configuration now that the game is reachable publicly.

---

## P5.4 — Production EOS Configuration

**Prerequisites / State Check:** `P5.3` complete. EOS dev sandbox from P1.4/P2.6 working.

**Objective:** Decide and configure the appropriate EOS sandbox/deployment for your public demo — either continue using the dev sandbox (acceptable for a college project's scale/purpose) or promote to a production-configured deployment — and ensure session advertisement correctly targets your now-public Oracle Cloud server.

**Expected Output:** EOS login/session flow (P2.6) working correctly against the production dedicated server, with session settings appropriately configured for public/demo use (e.g. a discoverable session name evaluators can find, or a fixed known session ID communicated directly if you prefer not to make it publicly searchable).

**Files to Modify:**
- `Game/Config/DefaultEngine.ini` (confirm/adjust EOS deployment settings for production use)
- `Docs/InstallationGuide.md`

**Implementation Steps:**
1. Decide: continue with the existing EOS Sandbox (reasonable — Epic's dev sandboxes are appropriate for this project's scale and don't require app review) vs. creating a distinct "Production" sandbox on the Epic Developer Portal for a cleaner separation from test data — either is defensible; document the choice.
2. Update session creation (`UEOSSessionSubsystem::CreateSession` from P2.6) with production-appropriate settings: a clear, findable session name/lobby name for evaluators, or alternatively a fixed connect flow if you'd rather share a specific IP/session ID directly rather than rely on public session search.
3. Test the full EOS login → find → join flow against the production dedicated server from an external network.
4. Document the exact steps an evaluator/player needs to follow to find and join your game in `Docs/InstallationGuide.md`.
5. Commit.

**Validation Checklist:**
- [ ] EOS login/session flow works correctly against the production server from an external client
- [ ] Session is discoverable/joinable following exactly the steps documented in `InstallationGuide.md`

**Testing Checklist:**
- [ ] Have someone else (not you) follow only the written instructions to find and join the session, with no additional help — this is the real test of whether the documentation is sufficient

**Git Commit Message:**
```
[P5.4] Finalize production EOS session configuration
```

**Documentation Updates:** `Docs/InstallationGuide.md` — "Finding and Joining a Game" section, evaluator-tested.

**Common Mistakes:**
- Assuming session discovery "just works" the same in production as it did during same-network dev testing — public internet NAT/routing conditions can behave differently; test from a genuinely separate network before trusting this.

**Next Prompt Dependency:** P5.5 packages the actual client build evaluators/players will run.

---

## P5.5 — Package Final Client Build

**Prerequisites / State Check:** `P5.4` complete.

**Objective:** Produce a clean, distributable Windows client package (the platform your evaluators will actually use) via `RunUAT BuildCookRun`, in `Shipping` or `Development` configuration (Development recommended for Month 1 — Shipping strips useful debug/log output you may still want during your live defense demo).

**Expected Output:** A `Builds/WindowsClient/` folder (or a zipped archive) containing a runnable `MonolithV.exe` and all required paks/dependencies, launchable on a machine that has never had the Unreal Editor installed.

**Files to Modify:**
- `Infra/cloud/package-client.sh` or `.ps1` (new — the exact packaging command, so it's repeatable)

**Implementation Steps:**
1. Run `RunUAT.bat BuildCookRun -project="Game/MonolithV.uproject" -platform=Win64 -clientconfig=Development -cook -build -stage -pak -archive -archivedirectory="Builds/WindowsClient"`.
2. Test the packaged build on a clean machine/VM (one without the Unreal Editor or any dev tools installed) — this is the real test of "is this actually distributable," not just "does it run on my dev machine."
3. Confirm the `DefaultEngine.Local.ini` EOS credentials pattern (from P1.4) is correctly baked in for this build (a shipped build needs its own valid EOS client credentials embedded — decide whether the same dev credentials are acceptable for a college demo, which they are, just confirm they're present in the packaged build's config, not accidentally excluded).
4. Zip the build folder for easy distribution/attachment to your submission.
5. Document the exact packaging command in `Infra/cloud/package-client.sh`.
6. Commit the script (not the build archive itself — too large for git; document where evaluators can obtain it, e.g. a release asset per P5.11).

**Validation Checklist:**
- [ ] Packaged build runs on a clean machine with no Unreal Engine installed
- [ ] EOS login/session join works correctly from this packaged build against the production server
- [ ] Build size is reasonable for distribution (check and document actual size)

**Testing Checklist:**
- [ ] Full playtest performed using only the packaged build (not PIE, not a Development Editor session) — this is the actual artifact evaluators will run, so it must be the thing actually tested

**Git Commit Message:**
```
[P5.5] Package final distributable client build
```

**Documentation Updates:** `Docs/InstallationGuide.md` — "Installing and Running the Game" section with exact steps for a fresh machine.

**Common Mistakes:**
- Only ever testing via Play-in-Editor or a Development Editor build throughout the whole project and discovering packaging problems for the first time here — this is exactly why P2.1 and earlier prompts insisted on testing against the real packaged builds early, not just PIE.

**Next Prompt Dependency:** P5.6 is the full end-to-end production smoke test using this exact packaged client.

---

## P5.6 — End-to-End Production Smoke Test

**Prerequisites / State Check:** `P5.5` complete.

**Objective:** One final, complete playthrough using only production artifacts — the packaged client (P5.5) against the Oracle Cloud dedicated server (P5.3) and backend/DB (P5.1/P5.2) — with no development shortcuts (no PIE, no localhost, no dev-only config), to prove the entire deployed system works as a whole before writing the final documentation.

**Expected Output:** A recorded, successful full playthrough (login → role select → climb → guardians → share → gate → checkpoints → death/respawn → leaderboard) entirely against production infrastructure, ideally from a genuinely separate physical machine/network than your dev machine.

**Files to Modify:**
- `Docs/Testing/TestPlan.md`

**Implementation Steps:**
1. On a separate machine if possible (or at minimum, ensure no dev-only shortcuts are active — no localhost overrides, no debug consoles left enabling cheats), run the packaged client, log in via EOS, join the production session.
2. Play the complete loop start to finish, exactly as a real evaluator/player would.
3. Record the session (screen capture) — this becomes both your test evidence and a candidate backup demo video for P5.10.
4. Log the result in `Docs/Testing/TestPlan.md` as "Phase 5 Production Smoke Test", with any issues found fixed immediately (this is the last chance before documentation/presentation prompts).
5. Commit.

**Validation Checklist:**
- [ ] Full playthrough succeeds entirely on production infrastructure with zero dev-only shortcuts active
- [ ] Recording captured successfully

**Testing Checklist:**
- [ ] This prompt's entire implementation IS the testing checklist

**Git Commit Message:**
```
[P5.6] Complete end-to-end production smoke test
```

**Documentation Updates:** `Docs/Testing/TestPlan.md` — final production smoke test entry.

**Common Mistakes:**
- Leaving a debug console cheat command active during this test (e.g. a leftover god-mode or infinite-fuel debug binding from earlier development) — audit and remove/disable all debug commands from the shipping build before this test, since their presence would also be a legitimate anti-cheat concern if discovered by an evaluator.

**Next Prompt Dependency:** P5.7 finalizes all documentation now that the production system is fully verified.

---

## P5.7 — Finalize All Documentation

**Prerequisites / State Check:** `P5.6` complete.

**Objective:** Do a complete pass over every `Docs/` file, ensuring each accurately reflects the final, production-deployed system (not an earlier in-progress state), with no dangling `TODO`/`Phase 2/3` forward-references that are now resolved.

**Expected Output:** `Docs/SRS.md`, `Docs/SDLC.md`, all `Docs/Architecture/*` diagrams, `Docs/Testing/*`, `Docs/RiskAnalysis.md`, `Docs/UserManual.md`, `Docs/InstallationGuide.md` all internally consistent, cross-referenced correctly, and accurate as of the final build.

**Files to Modify:** All files under `Docs/`.

**Implementation Steps:**
1. Re-read `Docs/SRS.md` end to end; confirm every FR is marked implemented with an accurate prompt-ID citation, remove/update any stale "planned" language.
2. Re-read `Docs/Architecture/*` diagrams; confirm the Class Diagram includes every major class that actually exists (Guardian, GAS abilities, world actors), not just the Phase 1 draft set.
3. Confirm `Docs/Testing/TestPlan.md` and `PerformanceReport.md` are complete and internally consistent with each other.
4. Confirm `Docs/UserManual.md` and `Docs/InstallationGuide.md` describe the actual final production flow (EOS login, production IP/session, packaged client) — not earlier local-dev-only instructions.
5. Have `Docs/RiskAnalysis.md` reviewed once more for accuracy given everything learned during Phase 4/5.
6. Commit.

**Validation Checklist:**
- [ ] No stale "TODO Phase 2/3" or "planned, not yet implemented" language remains anywhere it shouldn't
- [ ] Every diagram reflects the actual final system
- [ ] Cross-references between documents (SRS ↔ Architecture ↔ Testing) are consistent

**Testing Checklist:**
- [ ] Read every `Docs/` file once, cold, as if you were an evaluator seeing it for the first time — note anything confusing and fix it

**Git Commit Message:**
```
[P5.7] Finalize all documentation to reflect production system
```

**Documentation Updates:** This prompt IS the documentation finalization pass.

**Common Mistakes:**
- Treating documentation as "already done" from earlier phases and skipping this pass — documentation written incrementally during a fast build accumulates small inaccuracies (renamed classes, changed decisions) that a dedicated final pass is specifically meant to catch.

**Next Prompt Dependency:** P5.8 does a final clean-machine verification of the installation guide specifically.

---

## P5.8 — Installation Guide Clean-Machine Verification

**Prerequisites / State Check:** `P5.7` complete.

**Objective:** Literally follow `Docs/InstallationGuide.md`, word for word, on a genuinely clean machine (a fresh VM, or a friend's computer), changing nothing that isn't explicitly written in the guide, to prove it's actually sufficient — this is the single best defense against "the demo didn't work during grading because of an undocumented local dependency."

**Expected Output:** A successful install-and-play following only the written guide, with any gaps found immediately fixed in the guide (not worked around silently).

**Files to Modify:**
- `Docs/InstallationGuide.md`

**Implementation Steps:**
1. On a clean Windows machine/VM (no dev tools, no manually-created config files beyond what the guide instructs), follow `Docs/InstallationGuide.md` exactly.
2. Note every point of friction or ambiguity, no matter how small.
3. Fix the guide immediately for each issue found, then re-verify the fixed instruction actually resolves it (don't just assume the fix works).
4. Repeat until a completely clean, guide-only install succeeds.
5. Commit the corrected guide.

**Validation Checklist:**
- [ ] Clean-machine install succeeds following only the written guide
- [ ] Every friction point found during verification was fixed in the guide itself, not worked around informally

**Testing Checklist:**
- [ ] The clean-machine test itself, performed and passing, is the validation

**Git Commit Message:**
```
[P5.8] Verify and correct installation guide via clean-machine test
```

**Documentation Updates:** `Docs/InstallationGuide.md` — corrected and verified.

**Common Mistakes:**
- Performing this "verification" on your own dev machine with dev tools already present — this cannot catch missing-dependency issues; it must be a genuinely clean environment.

**Next Prompt Dependency:** P5.9 builds the presentation deck now that everything is finalized and verified.

---

## P5.9 — Presentation Deck

**Prerequisites / State Check:** `P5.8` complete.

**Objective:** Build the 10-15 slide deck per `README.md`'s Presentation Checklist, following the structure: problem → 4 pillars → architecture diagram → live demo → results/metrics → future work.

**Expected Output:** `Docs/Presentation/SlideOutline.md` fully written out (content/talking points per slide, even if the actual `.pptx`/slides file is built in your presentation tool of choice outside this repo), plus the actual exported diagram images needed.

**Files to Modify:**
- `Docs/Presentation/SlideOutline.md`
- `Docs/Presentation/exports/` (new — exported PNG/SVG versions of the Mermaid diagrams for use in slides, since most slide tools don't render Mermaid natively)

**Implementation Steps:**
1. Write the full slide-by-slide outline: Title → Problem/Motivation → Core Gameplay Loop (with a screenshot) → The 4 Pillars (one slide each, or one overview slide with a clear visual mapping) → Architecture Diagram (exported from Mermaid) → Live Demo (transition slide) → Performance/Results (pull directly from `PerformanceReport.md`) → Known Limitations & Future Roadmap → Q&A/Thank you.
2. Export each relevant Mermaid diagram to a static image (via the Mermaid CLI, or GitHub's rendered view + screenshot, or an online Mermaid live-editor export) into `Docs/Presentation/exports/` for use in whatever slide tool you build the actual deck in.
3. Explicitly include the slide mapping each of the 4 abstract pillars to its concretely shipped feature (per the README Presentation Checklist item) with a one-line "how we proved it" note per pillar (e.g. citing the P4.2 bandwidth numbers, the P4.3 memory soak test, the P2.7/P2.8 concurrency tests).
4. Commit the outline and exported images (build the actual `.pptx`/Google Slides deck separately using this outline as the script).

**Validation Checklist:**
- [ ] Outline covers all sections from the README's Presentation Checklist
- [ ] Each of the 4 pillars has a clear "here's the proof" citation to a specific test/prompt
- [ ] Diagram exports are legible as standalone images (not tiny/cropped)

**Testing Checklist:**
- [ ] Do a full dry run of the outline as if presenting it out loud, timing it, confirming it fits your allotted defense/presentation slot

**Git Commit Message:**
```
[P5.9] Create presentation deck outline and diagram exports
```

**Documentation Updates:** `Docs/Presentation/SlideOutline.md` — complete.

**Common Mistakes:**
- Writing an outline that just re-describes the abstract's original ambitions instead of what was actually built and measured — presentations that honestly show the real, smaller, well-tested system (with a clear future roadmap) come across as more credible than ones that oversell.

**Next Prompt Dependency:** P5.10 rehearses the actual demo and records the backup video this deck's "Live Demo" slide depends on.

---

## P5.10 — Demo Rehearsal & Backup Video

**Prerequisites / State Check:** `P5.9` complete.

**Objective:** Rehearse the live multiplayer demo enough times to be confident, and record a full backup video (per the README Presentation Checklist) in case the live network demo fails during your actual evaluation — a very real risk given you're demoing real networked infrastructure over whatever venue's internet/wifi is available.

**Expected Output:** At least 3 full rehearsal runs of the live demo script, and one clean, complete backup video recording covering the same content, stored/linked appropriately.

**Files to Modify:**
- `Docs/Presentation/DemoScript.md` (new — step-by-step script of exactly what you'll click/say/show, in order)

**Implementation Steps:**
1. Write `Docs/Presentation/DemoScript.md`: the exact sequence of actions for the live demo (e.g. "1. Show two laptops/instances connecting. 2. Both select opposite roles. 3. Both climb to band 2, fight a guardian together. 4. Perform the share interaction, point out the HUD confirming it. 5. Approach the share-gated checkpoint, show it unlocking. 6. Show the leaderboard after one player reaches the top.").
2. Rehearse this exact script at least 3 times, ideally on the actual venue's network/hardware if accessible beforehand, timing each run.
3. Record one full clean run as the backup video (screen capture, or a phone recording of two screens if demoing with two physical machines), covering the entire script.
4. Prepare a one-sentence contingency line for if live demo fails ("Let me switch to the recorded run captured during production testing") — rehearsed, not improvised, so it doesn't read as a stumble.
5. Commit the demo script (link to the video file/hosting location, don't commit large video binaries to git).

**Validation Checklist:**
- [ ] Demo script is precise enough that you don't need to improvise steps live
- [ ] Backup video successfully covers the full intended demo content
- [ ] Contingency plan rehearsed for a live-demo failure

**Testing Checklist:**
- [ ] At least 3 full rehearsals completed, timed, and refined based on what went wrong in earlier rehearsals

**Git Commit Message:**
```
[P5.10] Finalize demo script and record backup video
```

**Documentation Updates:** `Docs/Presentation/DemoScript.md` — complete.

**Common Mistakes:**
- Only ever demoing on your home network/dev machines and never considering venue wifi reliability for a networked multiplayer demo — this is exactly the scenario the backup video exists for; don't skip preparing it just because live rehearsals went well at home.

**Next Prompt Dependency:** P5.11 does the final public-repo polish before the actual publish/submission moment.

---

## P5.11 — GitHub Repository Polish

**Prerequisites / State Check:** `P5.10` complete.

**Objective:** Final pass making the public repository itself presentable as a portfolio piece — polished `README.md`, an appropriate open-source license, a tagged release with the packaged client build attached, and repo metadata (description, topics) set.

**Expected Output:** A GitHub repo that reads clearly to a stranger landing on it cold: what this is, how it's built, how to run it, and where to find the build.

**Files to Modify:**
- `README.md` (final polish pass — screenshots/GIFs from actual gameplay, not placeholder text)
- `LICENSE` (new)
- GitHub repo settings (description, topics/tags: `unreal-engine`, `multiplayer`, `csharp`, `oracle-database`, `game-development`)

**Implementation Steps:**
1. Add real screenshots/short GIFs from actual gameplay (P3.11's finished art level, the HUD, the share interaction) to `README.md` — a repo with zero visuals of the actual game is a weak first impression.
2. Choose and add an appropriate `LICENSE` file (MIT is a reasonable default for a portfolio/academic project unless your college has specific IP requirements — check if your institution has any constraints on licensing student project code before choosing).
3. Set the GitHub repo's description and topics via repo Settings for discoverability.
4. Create a GitHub Release (tag `v1.0.0-month1-vertical-slice` or similar), attaching the zipped packaged client build from P5.5 as a release asset (GitHub Releases handle large binaries better than committing them to the repo directly).
5. Do a final read-through of `README.md` as if you were a stranger discovering this repo via a link, with zero other context.
6. Commit final `README.md`/`LICENSE` changes; publish the Release separately (not a git commit, a GitHub Release action).

**Validation Checklist:**
- [ ] README includes real gameplay visuals
- [ ] LICENSE present and appropriate
- [ ] GitHub Release created with the packaged client attached
- [ ] Repo description/topics set

**Testing Checklist:**
- [ ] Ask someone unfamiliar with the project to read only the README and tell you what the project is and how to run it — confirm they can, without additional explanation

**Git Commit Message:**
```
[P5.11] Polish repository presentation and create v1.0.0 release
```

**Documentation Updates:** `README.md` — final polish complete.

**Common Mistakes:**
- Committing the large packaged client `.zip` directly into git history instead of using GitHub Releases — bloats repo size permanently even if later removed.

**Next Prompt Dependency:** P5.12 is the actual public launch/submission moment.

---

## P5.12 — Public Launch & Submission Packaging

**Prerequisites / State Check:** `P5.11` complete. This is a genuinely public, external-facing action (making the repo/release fully public if it wasn't already, posting publicly, submitting to your college) — confirm you're ready before proceeding, since undoing a public announcement is not clean.

**Objective:** Execute the actual public launch: confirm repo visibility, publish the milestone Twitter/X post, and assemble the final college submission package.

**Expected Output:** A live public GitHub repository and Release, a published Twitter/X post/thread announcing the finished vertical slice, and a complete submission package (repo link, packaged build link, all `Docs/` deliverables) ready to hand to evaluators.

**Files to Modify:**
- None (this is primarily an action/checklist prompt, not a code change) — optionally, a `Docs/SubmissionPackage.md` summarizing exactly what's being submitted and where to find each piece.

**Implementation Steps:**
1. Double-check repo visibility is set to Public (should already be true since P1.1) and that the Release from P5.11 is published, not draft.
2. Post your milestone Twitter/X update announcing the finished project (per your established 2-3x/week milestone cadence) — this is a genuinely public action; write your own copy per your earlier preference (Prompt Book only flags the moment).
3. Assemble `Docs/SubmissionPackage.md`: a single-page index linking to the repo, the Release/build, and each key `Docs/` deliverable (SRS, SDLC, diagrams, test plan, performance report, risk analysis, user manual, installation guide) — the one document you hand an evaluator first.
4. Perform the actual college submission per your institution's process, attaching/linking everything from `Docs/SubmissionPackage.md`.
5. Commit `Docs/SubmissionPackage.md`.

**Validation Checklist:**
- [ ] Repo and Release are genuinely public and accessible
- [ ] Twitter/X post published
- [ ] `Docs/SubmissionPackage.md` correctly links every required deliverable
- [ ] Actual college submission completed per institutional process

**Testing Checklist:**
- [ ] Open every link in `Docs/SubmissionPackage.md` in a private/incognito browser window to confirm they're genuinely publicly accessible, not accidentally gated behind your own logged-in session

**Git Commit Message:**
```
[P5.12] Finalize submission package and public launch
```

**Documentation Updates:** `Docs/SubmissionPackage.md` — created and complete.

**Common Mistakes:**
- Assuming a link "works" because it works while logged into your own GitHub account — always verify public accessibility in a private browsing session, since private repos/draft releases look fine to the owner but are inaccessible to evaluators.

**Next Prompt Dependency:** P5.13 is the final verification/handoff checklist closing the entire Prompt Book.

---

## P5.13 — Final Submission Checklist Verification

**Prerequisites / State Check:** `P5.12` complete. This is the last prompt in the entire Prompt Book.

**Objective:** Walk through every single checkbox in `README.md`'s Milestone Checklist, Presentation Checklist, and College Submission Checklist one final time, confirming each is genuinely, verifiably true — not "probably fine."

**Expected Output:** Every checkbox in `README.md` checked with a clear conscience, `PROJECT_STATE.md` showing the project as fully complete.

**Files to Modify:**
- `README.md` (check off every checklist item)
- `PROJECT_STATE.md`

**Implementation Steps:**
1. Go through `README.md`'s Milestone Checklist (M1-M5) one by one, confirming each against actual current repo/deployment state (not memory of having done it weeks ago — re-verify).
2. Go through the Presentation Checklist, confirming the deck, backup video, demo script, and diagrams all genuinely exist and are current.
3. Go through the College Submission Checklist, confirming every `Docs/` file is present, complete, and accurate.
4. Check every box in `README.md` that's genuinely satisfied; for anything not fully satisfied, either fix it now or make an explicit, documented decision to accept the gap (and note why).
5. Update `PROJECT_STATE.md` to reflect the fully complete state: `Last Completed Prompt ID: P5.13`, `Current Phase: Complete`, `Next Prompt To Run: None — project complete`.
6. Final commit, final tag (`v1.0.0-final-submission`), final merge `develop` → `main`.

**Validation Checklist:**
- [ ] Every Milestone Checklist item genuinely verified true
- [ ] Every Presentation Checklist item genuinely verified true
- [ ] Every College Submission Checklist item genuinely verified true

**Testing Checklist:**
- [ ] This entire prompt IS the final testing/verification pass for the whole project

**Git Commit Message:**
```
[P5.13] Complete final verification — project submission ready
```

**Documentation Updates:** `README.md` — all checklists checked; `PROJECT_STATE.md` — final complete state recorded.

**Common Mistakes:**
- Checking boxes based on "I did this weeks ago, it's probably still fine" instead of re-verifying against current repo/deployment state — things drift (a VM reboots and loses a non-persistent service, a dependency gets a breaking update); re-verify for real, right before submission.

**Next Prompt Dependency:** None — this is the final prompt in the Prompt Book. Congratulations: empty computer to fully finished, deployed, documented, presented, and submitted multiplayer game, solo, in one month. Everything genuinely out of scope for that constraint lives in `README.md`'s Future Roadmap, ready to pick back up with the same tool-agnostic, self-contained prompt structure whenever you continue past Month 1.
