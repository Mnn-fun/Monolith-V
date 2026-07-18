# Architecture Components

- **Backend API (ASP.NET Core, C#)**: Owns REST endpoints for auth, profile, season, and atomic share/gate transactions.

- **Deployment Target**: MonolithV is hosted on the Oracle Cloud Infrastructure (OCI) Free Tier. Compute workloads (Unreal Engine dedicated server and ASP.NET Core API) execute on an Always-Free Ampere ARM VM (`VM.Standard.A1.Flex`, 2 OCPUs, 12 GB RAM, Canonical Ubuntu 22.04 LTS). Persistent relational state (accounts, season progress, role selection, share events) is managed by an Oracle Autonomous Transaction Processing (ATP) serverless instance using the `_medium` consumer group alias for balanced API concurrency and throughput.
