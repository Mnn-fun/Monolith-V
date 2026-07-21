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
