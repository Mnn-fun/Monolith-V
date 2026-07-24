# Anti-Cheat Audit Log

This document lists all client-originating inputs, server RPCs, and entry points, specifying what the client claims and how the server independently validates the intent.

## Client-Originating Entry Points

### 1. Movement Replication
*   **Client Claim:** "I moved to position X, Y, Z."
*   **Server Validation:** Unreal Engine's `UCharacterMovementComponent` replicates positions at 30Hz. If a client attempts to teleport or move faster than their current speed/acceleration configuration permits, the server rejects the location update and snaps the client back to the last known server-authoritative location.

### 2. GAS Ability Activation (e.g. `GA_TestAbility`)
*   **Client Claim:** "I activated TestAbility."
*   **Server Validation:** The Gameplay Ability System (GAS) executes ability logic server-side. The server validates if the ability is blocked by active tags, checks that the player has sufficient attributes (e.g., fuel/health cost), and verifies that the ability is not on cooldown. Any invalid execution is blocked.

### 3. Share-Item Request (`ServerRequestShareItem`)
*   **Client Claim:** "I want to share an item with the counterpart player."
*   **Server Validation:**
    *   **Proximity Check:** The server calculates the distance (`FVector::Dist`) between the giver and receiver character locations. If the distance exceeds 500 units, the request is rejected.
    *   **Rate-Limiting:** The server rejects requests from the same player connection if triggered within 1.0 second of the last request (`LastShareRequestTime`).
    *   **Role Constraint (Backend-Enforced):** The C# backend checks that both players have selected opposite gender roles in the database.

### 4. Season Role Selection (`ServerSubmitRoleChoice`)
*   **Client Claim:** "I want to select role X (MALE/FEMALE) for this season."
*   **Server Validation:**
    *   **Database Constrained:** The C# backend ensures the role is strictly either "MALE" or "FEMALE".
    *   **Idempotency / Single Choice:** The database schema has a primary key `(player_id, season_id)` on `player_season_roles`. Any subsequent attempt to assign a role will fail database validation (Unique Constraint ORA-00001) and be rejected.
