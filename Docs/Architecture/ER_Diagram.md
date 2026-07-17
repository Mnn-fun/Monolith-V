# MonolithV — Oracle Database Entity Relationship (ER) Diagram

This document describes the Version 1 relational database schema deployed on **Oracle Autonomous Transaction Processing (ATP)** (`_medium` consumer group).

---

## 🗺️ Visual ER Diagram (`Mermaid`)

```mermaid
erDiagram
    PLAYERS {
        string player_id PK "UUID/string primary key"
        string eos_account_id UK "Unique Epic Online Services Account ID"
        string display_name "Player display name"
        timestamp created_at "Account creation timestamp"
    }

    SEASONS {
        string season_id PK "Season identifier"
        number season_number UK "Monotonically increasing season cycle"
        timestamp started_at "Season start timestamp"
        timestamp ends_at "Season end timestamp (nullable)"
        number is_active "0 = Inactive, 1 = Active"
    }

    PLAYER_SEASON_ROLES {
        string player_id PK,FK "Ref: PLAYERS"
        string season_id PK,FK "Ref: SEASONS"
        string role "CHECK IN ('MALE', 'FEMALE')"
    }

    SHARE_EVENTS {
        string share_event_id PK "Event identifier"
        string season_id FK "Ref: SEASONS"
        string giver_player_id FK "Ref: PLAYERS (Giver)"
        string receiver_player_id FK "Ref: PLAYERS (Receiver)"
        string item_type "CHECK IN ('GOLDEN_APPLE', 'COUNTERPART_ITEM')"
        timestamp shared_at "Timestamp when share occurred"
    }

    CHECKPOINT_PROGRESS {
        string player_id PK,FK "Ref: PLAYERS"
        string season_id PK,FK "Ref: SEASONS"
        number checkpoint_index PK "Ordinal checkpoint number"
        timestamp reached_at "Timestamp when checkpoint was reached"
    }

    PLAYERS ||--o{ PLAYER_SEASON_ROLES : "selects role in"
    SEASONS ||--o{ PLAYER_SEASON_ROLES : "has roles assigned"

    PLAYERS ||--o{ SHARE_EVENTS : "gives item"
    PLAYERS ||--o{ SHARE_EVENTS : "receives item"
    SEASONS ||--o{ SHARE_EVENTS : "contains share events"

    PLAYERS ||--o{ CHECKPOINT_PROGRESS : "reaches checkpoints"
    SEASONS ||--o{ CHECKPOINT_PROGRESS : "tracks progress in"
```

---

## 📑 Table Summary & Design Constraints

### 1. `PLAYERS`
* **Purpose:** Core identity table keyed by internal `PLAYER_ID`.
* **Constraints:** `EOS_ACCOUNT_ID` is `UNIQUE NOT NULL` to prevent duplicate account registration from the same Epic account.

### 2. `SEASONS`
* **Purpose:** Manages temporal game cycles.
* **Constraints:** `SEASON_NUMBER` is `UNIQUE NOT NULL`. `IS_ACTIVE` enforces a boolean check (`0` or `1`).

### 3. `PLAYER_SEASON_ROLES`
* **Purpose:** Enforces the core game rule: **Each player selects exactly one role per season**.
* **Constraints:** Composite primary key (`PLAYER_ID`, `SEASON_ID`). `ROLE` has a strict check constraint (`ROLE IN ('MALE', 'FEMALE')`).

### 4. `SHARE_EVENTS`
* **Purpose:** The transactional audit log for the **Golden Apple / Counterpart Item** cooperation mechanic. Gate mechanics query this table to confirm whether two players have exchanged counterpart items before allowing checkpoint traversal.
* **Constraints:**
  * `ITEM_TYPE IN ('GOLDEN_APPLE', 'COUNTERPART_ITEM')`
  * `GIVER_PLAYER_ID <> RECEIVER_PLAYER_ID` (prevents self-sharing)
* **Performance Indexing:** Explicit composite index `ix_se_receiver_season` on `(receiver_player_id, season_id)` for sub-millisecond gate check queries during active gameplay.

### 5. `CHECKPOINT_PROGRESS`
* **Purpose:** Records sequential milestone completion within a season.
* **Constraints:** Composite primary key (`PLAYER_ID`, `SEASON_ID`, `CHECKPOINT_INDEX`).
