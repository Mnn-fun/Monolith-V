# Monolith-V Local Linux Development Environment (`P1.11`)

## Overview
This document outlines the provisioning and configuration steps for our local Ubuntu 22.04 LTS development environment (via **Windows Subsystem for Linux 2 — WSL2** / standard hypervisor VM). 
This environment mirrors the target Oracle Cloud Infrastructure (OCI) `ap-mumbai-1` (`Canonical Ubuntu 22.04 LTS`) runtime architecture from `provision-vm.sh`, providing a reproducible local sandbox for testing dedicated server binaries, containerized services (`Redis`), and `.NET` backend builds.

---

## 1. Environment & Base OS Provisioning
- **Hypervisor / Platform**: WSL2 on Windows 11 (`wsl --install -d Ubuntu-22.04`)
- **OS Release**: Canonical Ubuntu 22.04 LTS (`jammy`)
- **Kernel**: Linux `6.6.87.2-microsoft-standard-WSL2` `x86_64`
- **Default User**: `mann_unix` (sudo-enabled)

---

## 2. Package & Runtime Installation
To maintain complete parity with our Phase 5 cloud environment and run our local backend dependencies natively, the following packages are installed directly from canonical/official repositories:

```bash
# Update repository indexes and upgrade existing base system packages
sudo apt update && sudo apt upgrade -y

# Install core runtimes: .NET 8 SDK, Docker Engine, Docker Compose V2, Redis CLI, and UFW
sudo apt install -y dotnet-sdk-8.0 docker.io docker-compose-v2 redis-tools ufw
```

### Installed Runtime Verification
- **Docker**: `docker --version` (`Docker version 29.1.x`)
- **Docker Compose**: `docker compose version` (`Docker Compose version v2.40.x`)
- **.NET SDK**: `dotnet --version` (`8.0.x`)
- **Redis CLI**: `redis-cli --version` (`redis-cli 6.0.x/7.x`)

---

## 3. Firewall Configuration (`ufw`)
To mirror the cloud network security rules defined for our OCI instance (allowing SSH access on TCP 22 and Unreal Engine dedicated server game traffic on UDP 7777), the local firewall is configured consistently:

```bash
# Enable UFW and allow critical ports
sudo ufw allow 22/tcp comment 'SSH Administration'
sudo ufw allow 7777/udp comment 'UE5 Dedicated Server Game Port'
sudo ufw status verbose
```

---

## 4. Local Container & Cache Layer Verification (`docker compose`)
With Docker Engine active, we verify our local caching layer directly inside the Ubuntu environment using our project configuration at `/mnt/d/techathons/Sem-7 proj-seminar/Monolith-V/Infra/docker-compose.yml`:

```bash
# Navigate to project repository mount inside WSL2
cd "/mnt/d/techathons/Sem-7 proj-seminar/Monolith-V"

# Start the Redis cache-aside container in detached mode
sudo docker compose -f Infra/docker-compose.yml up -d

# Verify container status and port mapping (6379:6379)
sudo docker compose -f Infra/docker-compose.yml ps

# Confirm Redis daemon responsiveness inside Ubuntu via loopback
redis-cli -h localhost ping
# Expected Output: PONG
```

---

## 5. Parity & Drift Auditing vs. Oracle Cloud (`Phase 5`)
When transitioning from Phase 1 local development to Phase 5 OCI cloud deployment (`provision-vm.sh`), diffing this document against our live cloud state allows us to catch environment drift early:
- **OS Compatibility**: Both local and cloud instances run `Canonical Ubuntu 22.04 LTS`.
- **Firewall Consistency**: Both enforce open access only on `22/tcp` (`SSH`) and `7777/udp` (`Game Server UDP`).
- **Container Architecture**: Both execute the identical `Infra/docker-compose.yml` (`redis:7-alpine`) configuration.
