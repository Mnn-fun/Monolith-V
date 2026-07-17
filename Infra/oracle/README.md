# Oracle Autonomous Database (ATP) & Wallet Configuration

This directory houses documentation and local configuration notes for connecting **MonolithV** (`ASP.NET Core` backend & data scripts) to our **Oracle Autonomous Transaction Processing (ATP)** / Autonomous Database instance.

---

## 🔒 Security & Git Ignore Policy

> [!CAUTION]
> **NEVER commit your Oracle Wallet ZIP file (`Wallet_*.zip`), unzipped wallet directory (`Infra/oracle/wallet/`), or database passwords to Git!**
> Our `.gitignore` is pre-configured to ignore `Infra/oracle/wallet/` and `*.zip`. Always verify before pushing that no credential files (`cwallet.sso`, `tnsnames.ora`, `sqlnet.ora`) are staged.

---

## 📂 Wallet Setup Directory Structure

When obtaining credentials, place and unzip your wallet inside this directory under `wallet/`:

```text
Infra/oracle/
├── README.md               <-- This documentation
└── wallet/                 <-- (GITIGNORED) Unzipped Oracle Wallet contents
    ├── cwallet.sso
    ├── ewallet.p12
    ├── keystore.jks
    ├── sqlnet.ora
    ├── tnsnames.ora
    └── truststore.jks
```

---

## 🛠️ How to Obtain & Configure Your Wallet (For New Teammates)

1. **Log in to Oracle Cloud Infrastructure (OCI) Console.**
2. Navigate to **Oracle AI Database** -> **Autonomous Database** (or **Databases** -> **Autonomous Transaction Processing**).
3. Select the target database instance (e.g., `MonolithVDB`).
4. Click **Database connection** -> **Download wallet**.
5. Create a secure password for the wallet export (store this safely in a password manager; never in code).
6. Download the `Wallet_MonolithVDB.zip` file.
7. Extract the contents directly into:
   ```bash
   d:/techathons/Sem-7 proj-seminar/Monolith-V/Infra/oracle/wallet/
   ```

---

## 🔗 Connection Service Levels (`TNS Alias`)

Oracle Autonomous Database provides three pre-configured consumer groups (service levels) in `tnsnames.ora`:

| Service Level Alias | Performance Profile | Concurrency | Best Use Case in MonolithV |
| :--- | :--- | :--- | :--- |
| `monolithv_high` | Highest CPU & IO parallelization per query | Lowest (`~3` concurrent queries) | Heavy batch analytics, complex reporting, or massive migrations. |
| `monolithv_medium` | Balanced CPU/parallelism | Medium (`~20` concurrent queries) | **Default choice for MonolithV Backend API (`ASP.NET Core`)** — optimal throughput and connection pool stability. |
| `monolithv_low` | Serial execution (no parallelism) | Highest (`~300+` concurrent queries) | High-concurrency lightweight point lookups or rapid health pings. |

### Default Selection: `monolithv_medium`
For our initial vertical slice and general API traffic, **`monolithv_medium`** is our standard target connection string alias.

---

## ⚙️ Environment Variables Example (`ASP.NET Core`)

In your local `appsettings.Development.json` (or `.env.local`), reference the wallet path cleanly without exposing secrets:

```json
{
  "ConnectionStrings": {
    "OracleDb": "User Id=ADMIN;Password=YOUR_SECURE_PASSWORD;Data Source=monolithv_medium;TNS_ADMIN=d:\\techathons\\Sem-7 proj-seminar\\Monolith-V\\Infra\\oracle\\wallet"
  }
}
```

Make sure `TNS_ADMIN` points to the absolute directory containing `tnsnames.ora` and `sqlnet.ora`.
