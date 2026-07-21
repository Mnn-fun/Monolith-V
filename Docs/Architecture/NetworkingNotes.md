# Networking Notes

Running scratch file, folds into the sequence diagram later.

- **Networking Baseline:** 30Hz server tick (`NetServerMaxTickRate=30`), client prediction planned for Phase 2.

## Server Architecture Decision (P2.1)

**Choice:** Listen Server (Month-1 vertical slice) → Dedicated Server (post-college commercial release).

**Rationale:** The Epic Games Launcher binary distribution of UE 5.7 does not support building dedicated server targets (`MonolithVServer`). Building dedicated server binaries requires compiling UE5 from source (~150 GB, 2–4 hrs build time). For the Month-1 scope (solo developer, college submission deadline), a Listen Server provides identical networking semantics:

| Feature | Listen Server | Dedicated Server |
|---|---|---|
| Server-authoritative movement | ✅ | ✅ |
| `HasAuthority()` / `ROLE_Authority` | ✅ | ✅ |
| RPCs (Client→Server, Server→Client, Multicast) | ✅ | ✅ |
| `UCharacterMovementComponent` prediction/correction | ✅ | ✅ |
| GAS (`GameplayAbilities`) server-side execution | ✅ | ✅ |
| Replication (`DOREPLIFETIME`, `OnRep_*`) | ✅ | ✅ |
| Headless (no rendering on server) | ❌ | ✅ |
| Host player advantage (0ms latency) | Yes (mitigated by `NetServerMaxTickRate`) | No |

**Migration path:** `MonolithVServer.Target.cs` is already present. When UE5 is built from source, recompiling with `-Target=MonolithVServer` produces the headless dedicated server binary with zero C++ code changes. All `HasAuthority()`, RPC, and replication logic works identically.
