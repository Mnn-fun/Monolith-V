# Monolith-V User Manual

## Overview
This manual documents the player authentication, session discovery, and connectivity workflows for the Monolith-V client and authoritative dedicated server ecosystem.

---

## Logging In (Phase P2.6)

Monolith-V utilizes **Epic Online Services (EOS)** for cross-platform player authentication (`IOnlineIdentity`) and persistent unique identity management (`EOS_ProductUserId`).

### 1. Developer Authentication Tool Flow (Sandbox Testing)
During local development and pre-production testing, authentication is performed via the **EOS Developer Authentication Tool (`EOS_DevAuthTool.exe`)**:
1. Launch `EOS_DevAuthTool.exe` locally on port `8081`.
2. Configure two distinct test credentials (e.g., `PlayerAlpha` and `PlayerBeta`).
3. Launch Unreal Engine clients using command-line authentication arguments:
   ```powershell
   # Client 1 (PlayerAlpha)
   Monolith_V.exe -AUTH_LOGIN=localhost:8081 -AUTH_PASSWORD=PlayerAlpha -AUTH_TYPE=developer

   # Client 2 (PlayerBeta)
   Monolith_V.exe -AUTH_LOGIN=localhost:8081 -AUTH_PASSWORD=PlayerBeta -AUTH_TYPE=developer
   ```
4. Upon startup, `UEOSLoginSubsystem::Login()` authenticates against the local Dev Auth Tool and extracts the unique `EOS_ProductUserId` (e.g., `0002b80f...`), which serves as the canonical identity keyed in database transaction tables (`PLAYERS.EOS_ACCOUNT_ID`).

### 2. Production Account Portal Flow (Live Releases)
Once deployed out of the development sandbox, clients authenticate using the **Epic Account Portal** (`AuthType = "accountportal"`):
1. When `UEOSLoginSubsystem::Login("", "", "accountportal")` is invoked, the EOS SDK opens the system default web browser (or in-game overlay).
2. The user authenticates securely via their Epic Games account or linked identity provider (Steam, PlayStation, Xbox, Google, Apple).
3. Upon successful OAuth token exchange, the subsystem broadcasts `OnLoginComplete` with the verified `EOS_ProductUserId`.

---

## Joining Sessions via EOS

Monolith-V supports dynamic matchmaking and session discovery through `UEOSSessionSubsystem` (`IOnlineSession`):

1. **Session Advertisement (Dedicated Server)**:
   When `AMonolithVGameMode::StartPlay()` executes on an authoritative dedicated server (`HasAuthority() && IsRunningDedicatedServer()`), `UEOSSessionSubsystem::CreateSession(16, true)` registers the session with EOS backend services under search keyword `"MonolithV"`.

2. **Session Discovery (Client)**:
   A client invokes `UEOSSessionSubsystem::FindSessions()`, which queries the EOS matchmaking backend for active sessions matching `"MonolithV"`.

3. **Connecting & Traveling**:
   When a session is selected (`UEOSSessionSubsystem::JoinSession(SessionIndex)`), the EOS subsystem resolves the connection details (`GetResolvedConnectString()`) and initiates an automated client travel (`PC->ClientTravel(ConnectString, TRAVEL_Absolute)`) to connect directly to the authoritative server instance.

---

## Fallback Direct-IP Connection (Debug Path)

If EOS matchmaking backend services experience transient outages or during local network diagnostics, clients can connect directly via IP address using the console:
1. Open the developer console (`~` Tilde key).
2. Execute direct travel:
   ```text
   open 127.0.0.1:7777
   ```
   *(Or replace `127.0.0.1` with the authoritative server's public IP address).*
