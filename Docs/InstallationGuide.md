# Installation Guide

## EOS Setup

The project uses Epic Online Services (EOS) via the engine's built-in `OnlineSubsystemEOS` plugin. That plugin reads credentials from an `Artifacts` array under `[/Script/OnlineSubsystemEOS.EOSSettings]` — not flat `ProductId`/`ClientId` keys — so the entire config, including the secret, lives in a local override file that's never committed.

1. Create `game/Monolith_V/Config/UserEngine.ini` (gitignored, not created automatically on clone). This is the engine's actual per-user override file for Engine-type config (`ConfigHierarchy.h`'s `GameDirUser` layer, `{PROJECT}/Config/User{TYPE}.ini`) — a file named `DefaultEngine.Local.ini` is **not** part of the config hierarchy and is silently never loaded.
2. Add, using your own Epic Dev Portal Product/Sandbox/Deployment/Client credentials:
   ```
   [/Script/OnlineSubsystemEOS.EOSSettings]
   !Artifacts=ClearArray
   +Artifacts=(ArtifactName="",ClientId="<your Client ID>",ClientSecret="<your Client Secret>",ProductId="<your Product ID>",SandboxId="<your Sandbox ID>",DeploymentId="<your Deployment ID>",ClientEncryptionKey="")
   ```
   `ArtifactName` must be left **empty** — `UEOSSettings::GetSelectedArtifactSettings()` (engine source: `Engine/Plugins/Online/OnlineSubsystemEOS/Source/OnlineSubsystemEOS/Private/EOSSettings.cpp`) only auto-selects a named artifact if `DefaultArtifactName` is also set to match it; with a single artifact and no `-EpicApp=` launch argument, an empty `ArtifactName` is what the engine's own fallback path resolves to. The struct field is `ClientEncryptionKey`, not `EncryptionKey` (renamed in engine source; `EncryptionKey` is silently ignored).
3. Launch the editor and search the Output Log for `EOS Subsystem` to confirm it prints `OK`, not `NULL`.
