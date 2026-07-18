# Monolith-V Software Development Life Cycle (SDLC) & CI/CD

## Overview
This document defines the branching strategy, Continuous Integration (CI) practices, and manual verification protocols for the **Monolith-V** repository.

## Branching & Workflow (GitFlow)
- **`main`**: Production-ready branch (`Phase 1` base foundation and major milestone releases). Protected by branch rules requiring pull requests, restricted deletions, and status checks.
- **`develop`**: Active integration branch where feature branches (`feature/*`, `fix/*`, `docs/*`) merge.
- **Feature Branches**: Spun off `develop` (or `main` for initial scaffolds) and merged back via Pull Requests using the standard PR checklist.

## Continuous Integration (`.github/workflows/ci.yml`)
On every push and Pull Request targeting `main` or `develop`, our GitHub Actions workflow automatically executes:
1. **Environment Setup**: Provisioning `.NET 8` and `.NET 10` SDKs on `ubuntu-latest`.
2. **Backend Restoration & Build**: Running `dotnet restore` and `dotnet build Backend/MonolithV.Backend.sln --configuration Release`.
3. **Automated Testing**: Running `dotnet test Backend/MonolithV.Backend.sln --no-build --configuration Release` to ensure zero regressions in data access (`ODP.NET Core`), cache-aside logic (`StackExchange.Redis`), or API endpoints.

## Unreal Engine Compilation Check (Documented Limitation)
- **CI Limitation**: Full cloud-hosted Unreal Engine 5 builds in ephemeral GitHub hosted runners (`ubuntu-latest`) require multi-gigabyte engine caches, heavy compute/storage allocations, and custom container registry licensing. Setting up self-hosted UE5 CI runners is explicitly scoped out for Month 1.
- **Manual Verification Protocol**: Every Pull Request must manually verify Unreal project compilation before merge. This requirement is enforced via the checklist in `.github/PULL_REQUEST_TEMPLATE.md`:
  - `[ ] Unreal project compiles locally with zero errors (MonolithV.uproject opened/compiled via Unreal Editor or CLI)`
- **Future Revisit**: Automated UE5 headless build checks will be re-evaluated when dedicated self-hosted runner infrastructure (`Phase 5` cloud/local VM provisioning) is scaled up.
