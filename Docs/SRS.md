# Software Requirements Specification (SRS)
## Monolith-V: Vertical-Slice Multiplayer Architecture

**Document Version:** 1.0.0  
**Status:** Baseline Specification (Phase 1 Deliverable)  

---

## 1. Introduction

### 1.1 Purpose
This Software Requirements Specification (SRS) defines the architectural, functional, and non-functional requirements for **Monolith-V**, a vertical-slice cooperative multiplayer game and its supporting enterprise backend infrastructure. This document serves as the authoritative technical baseline for evaluators, developers, and architectural reviewers, detailing the concrete systems implemented across the project lifecycle (`Phase 1` through `Phase 5`).

### 1.2 Intended Audience
- **Academic Evaluators & Technical Reviewers:** To assess architectural rigor, distributed systems synchronization, and full-stack software engineering practices.
- **Project Developers & Systems Engineers:** To guide implementation, module decoupling, API specification, and testing protocols.

### 1.3 Definitions & Acronyms
- **Monolith:** The central vertical world structure serving as the primary climbing and progression environment.
- **Altitude Band:** A discrete vertical coordinate segment of the world (`Z-axis`) used to index, stream, and unload procedural terrain chunks.
- **Guardian:** Server-controlled AI entities defending critical progression bottlenecks within specific Altitude Bands.
- **Share-Gate:** A cooperative barrier requiring synchronized, atomic resource/energy transfers between multiple players to unlock higher Altitude Bands.
- **Season:** A server-tracked temporal cycle governed by global progress state and database checkpoints.
- **EOS:** Epic Online Services (providing matchmaking, session management, and peer connectivity).
- **OCI:** Oracle Cloud Infrastructure (`ap-mumbai-1` target deployment environment).
- **TTL:** Time-To-Live (cache expiration duration).

### 1.4 Scope
Monolith-V is a **vertical-slice cooperative multiplayer game** designed to demonstrate enterprise-grade game architecture within a strict 1-month development window. The scope is explicitly bounded to:
- A `2 to 4 player` cooperative vertical climbing experience running on a dedicated Linux server.
- Server-authoritative movement, jetpack physics, and combat replication at `30Hz`.
- Procedural vertical chunk streaming indexed by Altitude Bands.
- An atomic shared-state engine handling race conditions at cooperative Share-Gates.
- A decoupled `.NET 8/10` REST API backed by Oracle Cloud Autonomous Database (`ODP.NET Core`) and local/remote `Redis` (`StackExchange.Redis`) cache-aside persistence.

**Out of Scope (Future Roadmap):** Massive multiplayer concurrency (`100+ players`), global multi-region database sharding, complex persistent player-to-player trading economies, and full cross-platform console certification (refer to project `README.md` Future Roadmap for post-vertical-slice expansions).

---

## 2. Overall Description

### 2.1 Product Perspective
Monolith-V is a distributed software system comprising two tightly coupled but cleanly separated operational layers:
1. **Unreal Engine 5 (`v5.4/v5.5`) Client & Dedicated Server:** Responsible for real-time simulation, physics, replication, input prediction, and visual/audio rendering.
2. **ASP.NET Core Backend API (`v8.0/v10.0`) & Data Services:** Responsible for persistent player profile storage, session initialization, atomic share-event validation, and high-speed cache lookups.

### 2.2 Product Functions (Core Gameplay Loop)
- Players authenticate and select specialized cooperative roles upon joining a session.
- Players ascend the Monolith using physical movement and jetpack traversal mechanics, triggering procedural chunk loads as they enter higher Altitude Bands.
- Players encounter defensive Guardians and must coordinate combat actions verified authoritatively by the dedicated server.
- Players reach cooperative Share-Gates, executing atomic resource share events (`Share mechanic`) to validate progression and register persistence checkpoints via the backend API.

### 2.3 User Classes
- **Player:** The sole primary user class interacting with the client application to join multiplayer sessions, control character avatars, and progress through Altitude Bands.

### 2.4 Operating Environment
- **Client Application:** Microsoft Windows 11 (`64-bit`), DirectX 12 compatible GPU, running the compiled `MonolithV` Unreal Engine 5 client binary.
- **Game Dedicated Server:** Canonical Ubuntu Server 22.04 LTS (`x86_64` / `aarch64` Ampere A1.Flex), running headless Unreal Engine 5 server binaries at `30Hz`.
- **Backend API & Caching Layer:** Canonical Ubuntu Server 22.04 LTS (`Docker Engine` / `docker-compose-v2`), hosting containerized `.NET` web services and `Redis 7 Alpine` (`localhost:6379`).
- **Database Layer:** Oracle Cloud Infrastructure (OCI) Autonomous Database Serverless (`20 GB` storage, TLS/Wallet mutual authentication).

---

## 3. Functional Requirements

### 3.1 Pillar 1: Server-Authoritative Sync
- **`FR-1.1` [Server Tick & Replication Rate]:** The dedicated server shall simulate game state and broadcast network replication updates at a fixed frequency of exactly `30Hz` (`NetServerMaxTickRate=30`). *(Traces to: `P1.2`, `P2.1`)*
- **`FR-1.2` [Client Prediction & Reconciliation]:** The client application shall locally predict avatar movement and jetpack thrust instantly upon input, while accepting server-authoritative correction frames to reconcile position discrepancies. *(Traces to: `P2.2`)*
- **`FR-1.3` [Authoritative Combat & Hit Validation]:** All weapon traces, projectile collisions, and damage calculations shall be validated exclusively by the dedicated server to prevent client-side manipulation. *(Traces to: `P2.3`)*

### 3.2 Pillar 2: Procedural Memory Optimization
- **`FR-2.1` [Altitude-Indexed Chunk Streaming]:** The dedicated server and client shall dynamically load procedural terrain and structure chunks indexed by vertical `Z-axis` coordinate thresholds (Altitude Bands). *(Traces to: `P3.1`)*
- **`FR-2.2` [Bounded Memory Lifecycle Management]:** The streaming engine shall automatically unload lower Altitude Band chunks when all active players ascend beyond a defined buffer threshold, maintaining a strictly bounded memory footprint. *(Traces to: `P3.2`)*

### 3.3 Pillar 3: Atomic Shared-State Data Engine
- **`FR-3.1` [Concurrent Share-Event Validation]:** The system shall process simultaneous player share attempts at Share-Gates using transactional atomic validation to guarantee deterministic unlocking without race conditions or resource duplication. *(Traces to: `P2.6`, `P4.3`)*
- **`FR-3.2` [Distributed Checkpoint Synchronization]:** Upon successful Share-Gate completion, the dedicated server shall broadcast checkpoint state to all connected clients while initiating an asynchronous persistence request to the backend API. *(Traces to: `P2.6`)*

### 3.4 Pillar 4: Async Database & Cache Layer
- **`FR-4.1` [Asynchronous Non-Blocking Database Persistence]:** All database operations against the Oracle Cloud Autonomous Database (`ODP.NET Core`) shall execute via asynchronous tasks (`async/await`), ensuring zero blocking operations on the primary web API or game simulation threads. *(Traces to: `P1.7`, `P1.8`, `P4.1`)*
- **`FR-4.2` [Cache-Aside Profile Lookups]:** When retrieving player profile data (`/api/players/{id}`), the backend API shall query `StackExchange.Redis` (`6379`) before falling back to Oracle Database. Cache hits shall return immediately; cache misses shall populate Redis from Oracle with a `30-minute TTL`. *(Traces to: `P1.9`)*

### 3.5 Core Gameplay Requirements
- **`FR-5.1` [Role Selection & Ability Assignment]:** The session initialization flow shall permit players to select distinct cooperative roles, applying role-specific attribute modifiers and traversal capabilities. *(Traces to: `P2.4`)*
- **`FR-5.2` [Cooperative Share Mechanic]:** Players shall be able to initiate interactive resource/energy transfer events with adjacent cooperative teammates or Share-Gates. *(Traces to: `P2.5`)*
- **`FR-5.3` [Checkpoint Recovery & Respawn Routing]:** If a player's avatar reaches zero health or falls below the active Altitude Band threshold, the server shall respawn the avatar at the most recently validated checkpoint. *(Traces to: `P2.7`)*
- **`FR-5.4` [Guardian AI State Synchronization]:** Guardian AI entities shall execute behavior trees authoritatively on the server, replicating target acquisition, attack animations, and health state to clients. *(Traces to: `P3.3`)*
- **`FR-5.5` [Jetpack Traversal Physics & Replication]:** The character controller shall implement fuel-bounded jetpack vertical boost mechanics, synchronizing fuel consumption and velocity state across the network. *(Traces to: `P3.4`)*

---

## 4. Non-Functional Requirements

### 4.1 Performance & Scalability
- **`NFR-1` [Simulation Tick Stability]:** The dedicated server shall maintain a `30Hz` simulation loop with frame time jitter not exceeding `±3.3ms` under standard vertical-slice load (`2 to 4 concurrent players`).
- **`NFR-2` [API Latency]:** The ASP.NET Core backend API shall serve cached profile lookups (`Redis hit`) in under `15ms` (`P95`) and database fallback queries (`Oracle query`) in under `150ms` (`P95`).

### 4.2 Security & Data Integrity
- **`NFR-3` [Parameterized SQL Execution]:** All SQL queries within `OraclePlayerRepository` and related data access components shall utilize strict command parameterization (`OracleParameter`), totally preventing SQL injection vulnerabilities.
- **`NFR-4` [Server-Authoritative Anti-Cheat Baseline]:** Client applications shall never possess trust for position teleportation, inventory generation, or damage assignment; all state changes must pass server-side bounds checking.
- **`NFR-5` [Transport Layer Security]:** All communication between the ASP.NET Core backend and Oracle Cloud Autonomous Database shall be encrypted via TLS 1.2+ using mutual wallet authentication (`mTLS`).

### 4.3 Maintainability & Modular Architecture
- **`NFR-6` [Module Decoupling]:** The codebase shall enforce strict project separation matching the directory structure:
  - `Backend/MonolithV.Api/`: HTTP endpoints, request validation, and dependency injection wiring.
  - `Backend/MonolithV.Data/`: Repository interfaces (`CachedPlayerRepository`, `OraclePlayerRepository`), database drivers (`ODP.NET Core`), and Redis drivers (`StackExchange.Redis`).
  - `Backend/MonolithV.Tests/`: Unit and integration test suites executed via automated CI/CD.
  - `Source/MonolithV/`: Unreal Engine C++ game logic, actors, and network controllers.

---

## 5. System Constraints & Assumptions

### 5.1 Project Constraints
- **Resource Limits:** The production environment is constrained to the **Oracle Cloud Free Tier** allocation: `VM.Standard.A1.Flex` (`2 OCPUs`, `12 GB RAM`) and **Autonomous Database Serverless** (`20 GB` storage limit). All memory pooling, chunk unloading (`FR-2.2`), and cache TTLs (`FR-4.2`) must conform to these hard physical limits.
- **Timeline & Team Size:** The system is engineered for delivery within a `1-month` vertical-slice timeframe by a dedicated seminar team/solo engineer, prioritizing clean architectural separation and reliable core loops over content volume.

### 5.2 Architectural Assumptions
- **Network Topology:** Clients connect to the dedicated Linux server via UDP port `7777`; the dedicated server communicates with the REST API via TCP port `8080/5000` (or local container bridge network).
- **WSL2/Local Parity:** Day-to-day development relies on local Linux virtualization (`WSL2 Ubuntu 22.04 LTS`), assuming `100%` runtime compatibility with the target Oracle Cloud Ampere instance when container profiles (`redis:7-alpine`) and framework versions (`.NET 8/10`) match.
