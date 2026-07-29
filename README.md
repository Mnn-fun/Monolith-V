# Monolith-V

A server-authoritative, high-concurrency multiplayer vertical-combat game built in Unreal Engine 5 (C++), backed by an async C# (ASP.NET Core) service layer and Oracle Database, deployed to a headless Linux dedicated server on Oracle Cloud.

Players choose a seasonal role (male/female), climb altitude bands around a central **Monolith** that spawns guardian entities and traversal/combat equipment outward, fight with hybrid gun+ability combat, and race to reach the top before the season resets. A late-tower gate mechanic forces role-paired players to have shared a role-locked item (Golden Apple / counterpart item) at least once — turning solo competition into occasional forced cooperation.

This repository **is** the final-year project submission and **is** the public dev-log for a solo 1-month build. Everything here is real and running end-to-end at a small (vertical-slice) scale; the larger vision (infinite procedurally-themed tower, full seasons, matchmaking at scale) is documented in [Future Roadmap](#future-roadmap-post-month-1) and built on this same architecture without a rewrite.

---

## How to Use This Prompt Book

This project is designed to survive you switching AI coding tools mid-build (Claude hits a usage limit → you continue in Antigravity, GitHub Copilot, Cursor, whatever's available). That only works if **no tool needs to remember the conversation** — every tool only needs to read files that already exist in this repo. Here's the mechanism:

1. **`PROJECT_STATE.md`** (repo root) is the single source of truth for "where are we right now." It is updated at the end of every single prompt, never before. It records: current phase, last completed Prompt ID, timestamp, files touched, build/run status, known issues, and the exact next Prompt ID to execute.
2. **The 5 numbered files in `PromptBook/`** contain every prompt required to go from empty repo to published, defended, submitted project. Prompts are numbered `P<phase>.<index>` (e.g. `P2.4`). They build strictly in order within a phase, and phases build strictly in order.
3. **Every prompt is self-contained.** Each one opens with a "Prerequisites / State Check" block telling the agent exactly what must already exist and how to verify it (via `PROJECT_STATE.md` and `git log`), so a cold agent with zero conversation memory can pick up correctly. None of them say "as discussed" or "like before" — if it isn't written down in this repo, it doesn't count as decided.
4. **Git is the audit trail.** Every prompt's commit message is prefixed with its Prompt ID (`[P1.3] Add Oracle schema v1`). If `PROJECT_STATE.md` and the git log ever disagree, trust `git log` and fix `PROJECT_STATE.md` to match — never the reverse.

### Switching tools mid-prompt

1. Open `PROJECT_STATE.md`. Note `Last Completed Prompt ID`.
2. Open `PromptBook/0X_*.md`, find the **next** prompt after that ID.
3. Paste that single prompt's full text into the new tool as its task instructions, and point it at this repo.
4. The new tool reads the "Prerequisites / State Check" section, verifies the repo matches, and proceeds. It does not need any earlier chat history — the repo *is* the memory.
5. When the prompt is done, it updates `PROJECT_STATE.md` and commits, exactly like any other tool would.

This means Claude, Antigravity, and GitHub Copilot (or Copilot Workspace/Chat) are all interchangeable executors of the exact same instructions — there is no Claude-specific tool syntax anywhere in the Prompt Book, only plain engineering instructions (file paths, commands, code-level direction).

---

## Repository / Folder Structure

```
Monolith-V/
├── README.md                       # This file — master index & roadmap
├── PROJECT_STATE.md                # Living state file, updated after every prompt
├── .gitignore
├── .github/
│   └── workflows/
│       └── ci.yml                  # Build-check CI (Unreal compile + backend build/test)
│
│
├── Game/                           # Unreal Engine 5 project
│   ├── MonolithV.uproject
│   ├── Config/                     # DefaultEngine.ini, DefaultGame.ini, tick rate, EOS config
│   ├── Source/
│   │   ├── MonolithV/               # Shared client+server game module (C++)
│   │   │   ├── Player/              # Character, role selection, inventory
│   │   │   ├── Combat/              # Weapons, GAS abilities, damage
│   │   │   ├── AI/                  # Guardian behavior trees, spawner
│   │   │   ├── World/               # Altitude chunk streaming manager, Monolith actor
│   │   │   ├── Networking/          # Backend HTTP client, replication helpers
│   │   │   └── UI/                  # HUD, menus (UMG)
│   │   └── MonolithVServer/         # Dedicated server target overrides (if needed)
│   └── Content/                    # Art, maps, blueprints-for-tuning
│
├── Backend/                        # ASP.NET Core solution (C#)
│   ├── MonolithV.Backend.sln
│   ├── MonolithV.Api/               # REST endpoints: auth, profile, season, share/gate transactions
│   ├── MonolithV.Data/              # Async Oracle (ODP.NET Core) data layer, Redis cache layer
│   └── MonolithV.Tests/             # Unit/integration tests for transaction logic
│
├── Infra/
│   ├── docker-compose.yml          # Local dev: Redis + backend container
│   ├── oracle/
│   │   ├── schema/                 # DDL scripts, versioned migrations
│   │   └── seed/                   # Seed data for local dev
│   └── cloud/
│       ├── provision-vm.sh         # Oracle Cloud Free Tier VM provisioning
│       └── deploy-server.sh        # Pushes dedicated server build + backend to VM
│
└── Docs/
    ├── SRS.md
    ├── SDLC.md
    ├── Architecture/
    │   ├── ER_Diagram.md
    │   ├── Class_Diagram.md
    │   ├── Activity_Diagram.md
    │   ├── Sequence_Diagram.md
    │   ├── DFD.md
    │   └── Flowchart.md
    ├── Testing/
    │   └── TestPlan.md
    ├── RiskAnalysis.md
    ├── UserManual.md
    ├── InstallationGuide.md
    └── Presentation/
        └── SlideOutline.md
```

---

## Locked Technical Decisions

| Area | Decision |
|---|---|
| Engine | Unreal Engine 5 (latest 5.4/5.5), **C++ primary**, Blueprint only for tuning/VFX/UI wiring |
| Ability system | Gameplay Ability System (GAS) — hybrid gun + ability combat |
| Networking | Unreal built-in replication/NetDriver as the authoritative transport, tuned to **30Hz** server tick + client-side prediction/interpolation for feel |
| Online subsystem | Epic Online Services (EOS) — login, sessions |
| Backend | ASP.NET Core (C#), async all the way down — no blocking DB calls on any hot path |
| Database | Oracle Database (Oracle Autonomous DB Free Tier), accessed via async ODP.NET Core |
| Cache | Redis (session/profile cache, included from Month 1) |
| Hosting | Oracle Cloud Free Tier (Always-Free ARM VM for dedicated server + Autonomous DB) for final demo; local Linux VM for day-to-day dev |
| Git workflow | Full GitFlow (`main` / `develop` / `feature/*` / `release/*`) |
| CI | GitHub Actions — compile/package check only (no heavy test suite in Month 1) |
| Testing | Manual playtesting per milestone + targeted unit tests on transaction-validation logic (Phase 4) |
| Art style | Stylized (Genshin/Valorant-adjacent) — not photorealistic, not retro low-poly |
| Public build-in-public | GitHub commits + 2-3x/week Twitter/X posts at milestone boundaries (Prompt Book flags the moment; you write the copy) |

---

## Complete SDLC Roadmap

This project follows an **Incremental/Iterative SDLC** (not pure Waterfall, not full Scrum ceremony-for-ceremony — a solo-appropriate hybrid), because a 1-month solo timeline can't absorb a full requirements-freeze phase, but a final-year report still needs clean phase gates to grade against.

| SDLC Stage | Maps to | Output |
|---|---|---|
| Requirements & Feasibility | This discovery conversation + `Docs/SRS.md` | SRS, scoped vertical-slice definition |
| System Design | Phase 1 + `Docs/Architecture/*` | ER diagram, class diagram, architecture diagram |
| Implementation (Iteration 1: Core Systems) | Phase 2 | Working networking/DB/backend skeleton |
| Implementation (Iteration 2: Gameplay) | Phase 3 | Playable vertical slice |
| Verification & Validation | Phase 4 | Test plan, bug log, performance report |
| Deployment & Release | Phase 5 | Deployed build, installation guide, final docs |
| Maintenance (documented, not executed) | Future Roadmap section below | Phase 2+ backlog |

---

## Estimated Timeline (30 Days, Solo)

| Days | Phase | Focus |
|---|---|---|
| 1–4 | Phase 1 — Project Foundation | Repo/GitFlow, Unreal scaffold, EOS project, Oracle Cloud + DB provisioning, backend skeleton, Redis, CI, SRS draft |
| 5–10 | Phase 2 — Core Architecture | Dedicated server build, 30Hz replication + prediction, GAS setup, altitude chunk-streaming core, atomic transaction endpoints, async Oracle layer, Redis wiring, EOS login/session flow |
| 11–20 | Phase 3 — Game Development | Role selection + shareable items + gate mechanic, jetpack traversal, hybrid combat, guardian AI, checkpoints, Level 1 (full art) + Level 2 (greybox), HUD, seasonal reset stub, basic leaderboard |
| 21–25 | Phase 4 — Optimization & Testing | Profiling, bandwidth/interest management, memory/streaming validation, prediction tuning, soak test, server-side validation hardening, unit tests, CI hardening, bug bash |
| 26–30 | Phase 5 — Deployment & Publishing | Prod VM + DB deploy, Docker packaging, client packaging, docs finalization, presentation deck, GitHub polish, public publish, submission packaging |

---

## Milestone Checklist

- [ ] **M1** — Repo live on GitHub with GitFlow branches, CI green on empty scaffold (end of Phase 1)
- [ ] **M2** — Two Unreal clients connect to one dedicated server instance and see each other move, replicated at 30Hz, with client prediction masking latency (end of Phase 2)
- [ ] **M3** — Full solo climb loop playable: role select → jetpack up altitude chunks → fight 1 guardian archetype → hit a share-gated checkpoint with a second player → respawn on death (end of Phase 3)
- [ ] **M4** — Profiled, soak-tested build with no server-thread DB blocking, documented perf numbers, passing unit tests, green CI (end of Phase 4)
- [ ] **M5** — Publicly deployed on Oracle Cloud, installable by a stranger following `Docs/InstallationGuide.md`, full documentation set complete (end of Phase 5)

---

## Presentation Checklist

- [ ] 10–15 slide deck following `Docs/Presentation/SlideOutline.md` (problem → 4 pillars → architecture diagram → live demo → results/metrics → future work)
- [ ] Recorded backup demo video (in case live network demo fails in front of evaluators)
- [ ] Live demo script with a second machine/laptop or two Unreal client instances ready to show real multiplayer, not single-player-looking footage
- [ ] Architecture diagram printed/exportable as a single clear image (not a screenshot of code)
- [ ] One slide explicitly mapping abstract's 4 pillars → the actual shipped feature proving each one
- [ ] Answers prepared for the obvious question: "why isn't this using raw sockets / a bigger world / Redis-at-scale yet" → point to Future Roadmap

## College Submission Checklist

- [ ] `Docs/SRS.md` complete
- [ ] `Docs/SDLC.md` complete (this roadmap, formalized)
- [ ] All 6 diagrams present in `Docs/Architecture/`
- [ ] `Docs/Testing/TestPlan.md` with actual executed results, not just a template
- [ ] `Docs/RiskAnalysis.md` complete
- [ ] `Docs/UserManual.md` complete
- [ ] `Docs/InstallationGuide.md` verified by following it on a clean machine/VM
- [ ] Public GitHub repo link ready to share with evaluators
- [ ] Final `PROJECT_STATE.md` shows all 5 phases complete
- [ ] Packaged, runnable build (client + server) attached/linked in submission

---

## Future Roadmap (Post-Month-1)

Everything below is **architecturally supported** by the Month-1 build (no rewrite required) but intentionally not built yet, to keep the solo 1-month scope real:

- Infinite procedurally-themed tower generation (beyond the 2 levels built in Month 1) — the chunk-streaming system is built generic/parameterized specifically so new themed levels are data, not code
- Attachable slab traversal equipment (jetpack ships first)
- Full seasonal matchmaking, ranked leaderboards, and cross-season persistent cosmetics/economy
- Redis scaled out beyond single-instance session/profile caching (clustering, pub/sub for cross-server events)
- Squad-based climbing (currently solo + forced role-pairing only)
- Additional guardian archetypes and boss-tier guardians at higher altitude bands
- Anti-cheat hardening beyond server-authority (e.g. replay-based anomaly detection)
- Raw custom UDP transport layer (currently Unreal's NetDriver/replication graph) — a legitimate multi-week research extension if pursued post-graduation
- Full research-paper writeup of the 4 pillars (deferred by design — SRS/SDLC docs cover Month 1's academic requirement)

---

## Phase Index

1. [Project Foundation](PromptBook/01_Project_Foundation.md)
2. [Core Architecture](PromptBook/02_Core_Architecture.md)
3. [Game Development](PromptBook/03_Game_Development.md)
4. [Optimization & Testing](PromptBook/04_Optimization_Testing.md)
5. [Deployment & Publishing](PromptBook/05_Deployment_and_Publishing.md)
