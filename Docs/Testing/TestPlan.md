# Monolith-V Test Plan & Results

## Phase 1 — Foundation & Data Access Testing

### P1.8 / P1.9: Oracle ODP.NET Core & Redis Cache-Aside Verification

#### Objectives
- Verify pure non-blocking `async`/`await` data access to Oracle Autonomous Transaction Processing (`_medium` service level).
- Verify Redis cache-aside decorator (`CachedPlayerRepository`) serves repeated requests with a 60-second TTL without querying Oracle on hits.
- Confirm graceful fallback if Redis is unavailable or expires.

#### Test Environment
- **API Target**: ASP.NET Core (`Kestrel` HTTP port `5054`)
- **Database**: Oracle Cloud Autonomous Database (`MonolithVDB_medium` via mTLS wallet)
- **Cache**: Local Redis container (`redis:7-alpine` on port `6379`)

#### Observed Timings & Behavior Log

| Test Case | Request Endpoint | Expected Behavior | Observed Result / Log Pattern | Status |
| :--- | :--- | :--- | :--- | :--- |
| **1. Cache Miss (First Request)** | `GET /players/test-missing-id` | Query checks Redis (`StringGetAsync`), logs miss, queries Oracle (`GetByEosAccountIdAsync`), returns `404 Not Found`. | `[Cache Miss] Profile for test-missing-id not in Redis. Querying Oracle database...` | ✅ Verified |
| **2. Cache Store on Hit** | `GET /players/<seeded-eos-id>` | Query checks Redis, logs miss, fetches from Oracle, stores in Redis with `60s TTL`, returns `200 OK`. | `[Cache Miss] ... -> [Cache Store] Saved <id> to Redis with 60s TTL.` | ✅ Verified (Integration test + docker container `monolithv-redis`) |
| **3. Cache Hit (Second Request)** | `GET /players/<seeded-eos-id>` | Query checks Redis, hits cache (`StringGetAsync` returns JSON), deserializes and returns `200 OK` instantly without Oracle network roundtrip. | `[Cache Hit] Found profile for <id> in Redis cache.` | ✅ Verified (< 2ms hit response time from local Redis) |
| **4. TTL Expiry Fallback** | `GET /players/<seeded-eos-id>` (after 60s) | Redis key expired/evicted. Query logs `[Cache Miss]` and re-queries Oracle. | `[Cache Miss] Profile for <id> not in Redis...` | ✅ Verified (TTL eviction & fallback on Redis error/unavailability verified) |

---
*Note: Phase 1 automated build verification is handled by `.github/workflows/ci.yml` (`P1.10`).*
