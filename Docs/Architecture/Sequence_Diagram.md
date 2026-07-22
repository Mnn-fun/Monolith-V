# Monolith-V Architecture Sequence Diagrams

## Server-Authoritative Character Movement Replication (`P2.2`)

The following diagram illustrates the client-predicted, server-authoritative movement flow using Unreal Engine 5's `UCharacterMovementComponent` and `UEnhancedInputComponent`:

```mermaid
sequenceDiagram
    autonumber
    actor Client as Client (Player)
    participant Pawn as Client Pawn (AMonolithVCharacter)
    participant CMC as Client UCharacterMovementComponent
    participant Server as Server (Listen/Dedicated)
    participant ServerCMC as Server UCharacterMovementComponent
    actor OtherClient as Other Connected Clients

    Client->>Pawn: Move Input (WASD / IA_Move)
    Pawn->>CMC: AddMovementInput(Forward/Right, Value)
    CMC->>CMC: Perform Client-Side Prediction (Local Move)
    CMC->>ServerCMC: Send SavedMove RPC (Timestamp, Acceleration, Position)
    
    Note over Server,ServerCMC: Server owns authoritative position & physics
    ServerCMC->>ServerCMC: Replay Move & Validate Position (Check for speed/position cheats)
    
    alt Position Valid
        ServerCMC-->>CMC: AckMove RPC (Client prediction confirmed)
        ServerCMC->>OtherClient: Replicate Authoritative Movement (ReplicatedMovement)
    else Position Discrepancy / Cheat Detected
        ServerCMC-->>CMC: ClientAdjustPosition RPC (Authoritative correction)
        Note over CMC: Client interpolates/snaps back to server position
        ServerCMC->>OtherClient: Replicate Corrected Position
    end
```
```

## Atomic Share-Event Dual-Layer Concurrency Flow (`P2.7`)

The following sequence diagram illustrates the concurrency-safe, race-condition-proof transaction pipeline (`POST /seasons/{seasonId}/share-events`) recording a Golden Apple / Counterpart Item share between two players. It demonstrates the dual-layer defense-in-depth architecture: keyed in-memory serialization (`SemaphoreSlim`) to eliminate database contention across identical concurrent requests, combined with database-level uniqueness enforcement (`ORA-00001` unique constraint on `SHARE_EVENTS(season_id, giver_player_id, receiver_player_id, item_type)`) as the absolute single source of truth across horizontal scaling boundaries:

```mermaid
sequenceDiagram
    autonumber
    actor ClientA as Concurrent Request A (Client 1)
    actor ClientB as Concurrent Request B (Client 2)
    participant API as ShareEventsController
    participant Repo as ShareEventRepository
    participant Lock as Keyed SemaphoreSlim (In-Process)
    participant DB as Oracle Database (SHARE_EVENTS & PLAYER_SEASON_ROLES)

    Note over ClientA,ClientB: Near-simultaneous share attempts for identical (SeasonId, GiverId, ReceiverId, ItemType)
    ClientA->>API: POST /seasons/s1/share-events { Giver, Receiver, GOLDEN_APPLE }
    ClientB->>API: POST /seasons/s1/share-events { Giver, Receiver, GOLDEN_APPLE }
    API->>Repo: RecordShareEventAsync(s1, Giver, Receiver, GOLDEN_APPLE)
    API->>Repo: RecordShareEventAsync(s1, Giver, Receiver, GOLDEN_APPLE)

    Note over Repo,Lock: Keyed lock: lockKey = "s1:Giver:Receiver:GOLDEN_APPLE"
    Repo->>Lock: WaitAsync() [Request A acquires lock immediately]
    Repo->>Lock: WaitAsync() [Request B blocks waiting for Request A]

    Note over Repo,DB: Request A executes guarded critical section
    Repo->>DB: SELECT role FROM player_season_roles WHERE season_id = s1 AND player_id IN (Giver, Receiver)
    DB-->>Repo: Returns opposite roles ('MALE' vs 'FEMALE')
    Repo->>DB: BEGIN TRANSACTION -> INSERT INTO share_events (share_event_id, s1, Giver, Receiver, GOLDEN_APPLE)
    DB-->>Repo: 1 row inserted (uq_share_events_atomic satisfied) -> COMMIT
    Repo->>Lock: Release() [Request A unlocks semaphore]
    Repo-->>API: ShareEventResult(Success: true, AlreadyShared: false, ShareEventId: ...)
    API-->>ClientA: 200 OK { success: true, alreadyShared: false, shareEventId: "..." }

    Note over Repo,DB: Request B unblocks and enters critical section
    Repo->>DB: SELECT role FROM player_season_roles WHERE season_id = s1 AND player_id IN (Giver, Receiver)
    DB-->>Repo: Returns opposite roles ('MALE' vs 'FEMALE')
    Repo->>DB: BEGIN TRANSACTION -> INSERT INTO share_events (share_event_id, s1, Giver, Receiver, GOLDEN_APPLE)
    Note over DB: uq_share_events_atomic UNIQUE constraint triggers!
    DB-->>Repo: Throws OracleException ORA-00001 (Unique Constraint Violation) -> ROLLBACK
    Note over Repo: Catch IsUniqueConstraintViolation(ex) -> Idempotent no-op treatment
    Repo->>Lock: Release() [Request B unlocks semaphore]
    Repo-->>API: ShareEventResult(Success: true, AlreadyShared: true, ShareEventId: null)
    API-->>ClientB: 200 OK { success: true, alreadyShared: true, message: "Share event already recorded." }
```

## Phase P2.8 — Atomic Checkpoint-Claim Transaction Sequence (`POST /seasons/{seasonId}/players/{playerId}/checkpoints`)

```mermaid
sequenceDiagram
    autonumber
    actor ClientA as Dedicated Server (Thread 1)
    actor ClientB as Dedicated Server (Thread 2)
    participant API as CheckpointsController
    participant Repo as CheckpointRepository
    participant Lock as SemaphoreSlim (season:player:checkpointIndex)
    participant DB as Oracle Database (CHECKPOINT_PROGRESS)

    Note over ClientA,ClientB: Both threads attempt to record checkpoint index 1 simultaneously
    ClientA->>API: POST /seasons/s1/players/pA/checkpoints { checkpointIndex: 1 }
    ClientB->>API: POST /seasons/s1/players/pA/checkpoints { checkpointIndex: 1 }

    API->>Repo: RecordCheckpointProgressAsync("s1", "pA", 1)
    Repo->>Lock: WaitAsync("s1:pA:1")
    Note over Repo,Lock: Request A acquires semaphore lock; Request B suspends waiting

    Note over Repo,DB: Request A executes critical section
    Repo->>DB: BEGIN TRANSACTION -> INSERT INTO checkpoint_progress (player_id, season_id, checkpoint_index, reached_at)
    DB-->>Repo: 1 row inserted (pk_checkpoint_progress satisfied) -> COMMIT
    Repo->>Lock: Release() [Request A unlocks semaphore]
    Repo-->>API: CheckpointClaimResult(Success: true, AlreadyClaimed: false)
    API-->>ClientA: 200 OK { success: true, alreadyClaimed: false, checkpointIndex: 1 }

    Note over Repo,DB: Request B unblocks and enters critical section
    Repo->>DB: BEGIN TRANSACTION -> INSERT INTO checkpoint_progress (player_id, season_id, checkpoint_index, reached_at)
    Note over DB: pk_checkpoint_progress (player_id, season_id, checkpoint_index) triggers!
    DB-->>Repo: Throws OracleException ORA-00001 (Unique Constraint Violation) -> ROLLBACK
    Note over Repo: Catch IsUniqueConstraintViolation(ex) -> Idempotent no-op treatment
    Repo->>Lock: Release() [Request B unlocks semaphore]
    Repo-->>API: CheckpointClaimResult(Success: true, AlreadyClaimed: true)
    API-->>ClientB: 200 OK { success: true, alreadyClaimed: true, message: "Checkpoint already claimed." }
```
