# Monolith-V — Phase 3 Complete 3D Asset Reference Guide

> This document covers every art and 3D asset required across Phase 3 of development, organized by when you need it (by prompt order), whether to model it in Blender or source from Fab/Quixel, and licensing clarity for enterprise/commercial use.

---

## 📜 FAB STORE LEGALITY — Read This First

| Usage Type | Fab / UE Marketplace Assets | Quixel Megascans |
|---|---|---|
| **Student Project / Non-Commercial** | ✅ 100% Legal | ✅ Free & Legal (bundled with UE5) |
| **Commercial Game Release** | ⚠️ Must verify each asset uses **Standard UE Marketplace License** (most do) | ✅ Free for ALL UE projects including commercial |
| **Reselling the raw assets themselves** | ❌ Never permitted | ❌ Never permitted |

### Key Rules
- Always filter Fab searches by **"Standard License"** — this is the license that grants commercial game distribution rights.
- **Quixel Megascans** (already integrated in UE5 via the Bridge plugin) are free for any project built in Unreal Engine, including commercial releases — use these freely for all environment fill.
- **Mixamo** (Adobe) provides free character animations exportable as FBX — free for personal and commercial use.

---

## 🗓️ Asset Timeline — Organized by Development Order

---

### Before P3.1 — Role Selection UI

No 3D assets required at this stage. Only 2D icon artwork is needed for the role selection menu buttons.

| Asset | Type | Source |
|---|---|---|
| Male Role Icon (2D) | UI Texture (PNG/SVG) | Create in Canva, Figma, or any icon pack |
| Female Role Icon (2D) | UI Texture (PNG/SVG) | Create in Canva, Figma, or any icon pack |

---

### Before P3.2 — Golden Apple & Counterpart Item

Two small prop meshes required for the core cooperative mechanic items.

| Asset | Description | Source | Priority |
|---|---|---|---|
| **Golden Apple** | Glowing golden apple prop (~15–20 cm scale). Used as the Male role's shareable item | Model in **Blender** (~30–45 min) OR Fab: search `"golden apple prop UE5"` | 🔴 Required |
| **Silver Blossom / Counterpart Item** | Female counterpart — a silver flower, crystal orb, or glowing bloom | Model in **Blender** OR Fab: `"crystal orb prop"` / `"glowing flower prop"` | 🔴 Required |

> **Tip:** These are tiny props but appear prominently in the HUD and world. Keep polygon count low (~500–1000 tris). Add a simple emissive glow material in UE5's Material Editor — no extra purchase needed.

---

### Before P3.4 — Jetpack Traversal & Player Characters

This is the most critical character art decision of the entire project.

| Asset | Description | Source | Notes |
|---|---|---|---|
| **Male Player Character** (Skeletal Mesh) | Fully rigged humanoid with UE5 Mannequin-compatible skeleton | **Option A (Recommended):** Use UE5's built-in `SKM_Manny` with stylized material. **Option B:** Fab search `"stylized male character UE5 standard license"` | Skeleton MUST be UE5 Mannequin-compatible for standard animations to retarget |
| **Female Player Character** (Skeletal Mesh) | Fully rigged humanoid, female variant | **Option A:** Use UE5's built-in `SKM_Quinn` with stylized material. **Option B:** Fab search `"stylized anime female character UE5"` | Same skeleton requirement |
| **Jetpack Prop** (Static or Skeletal Mesh) | Backpack thruster attached to character spine/back socket | Model in **Blender** (simple box + exhaust nozzle shape, ~30 min) OR Fab: `"jetpack prop UE5"` | Attach to a `spine_03` or custom back socket on character rig |
| **Player Animations** | Idle, Walk, Run, Jump, Fall, Land | Already included with `SKM_Manny`/`SKM_Quinn` UE5 default content, OR **Mixamo.com** (free) | Export from Mixamo as FBX → Import → Retarget to your skeleton |

---

### Before P3.5 — Ranged Weapon

One weapon mesh with a defined muzzle socket for hit-scan ray origin.

| Asset | Description | Source | Notes |
|---|---|---|---|
| **Stylized Rifle / Pistol** (Static or Skeletal Mesh) | Single primary weapon mesh | **Option A:** UE5 **Lyra Starter Game** includes a free rifle mesh redistributable in other UE projects. **Option B:** Fab: `"stylized sci-fi weapon UE5 free"`. **Option C:** Model in Blender (~1–2 hrs) | Must have a `Muzzle` socket at the barrel tip for trace/VFX origin |
| **Muzzle Flash VFX** | Niagara particle burst at weapon muzzle on fire | UE5 built-in Niagara template `"Sprite Burst Instant"` — free, already in engine | No purchase needed |

---

### Before P3.7 — Guardian Enemy

The Guardian is one of the two hero assets your PromptBook explicitly calls out as the focus of every screenshot and demo clip.

| Asset | Description | Source | Priority |
|---|---|---|---|
| **Guardian Character** (Skeletal Mesh) | The main enemy pawn. Must be visually distinct from player characters. Recommended style: armored stone sentinel, otherworldly metal construct, or imposing humanoid warrior | **Custom model in Blender** — this is explicitly the recommended approach in the project plan. Leverage advanced Blender skills here | 🔴 Critical — Model this yourself |
| **Guardian Animations** | Idle patrol, Walk/Chase, Attack swipe, Death | **Option A:** If humanoid rig → retarget UE5 Mannequin animations (free). **Option B:** **Mixamo.com** (free) — "Zombie Walk", "Punching", "Dying Backward" all work well for a guardian. Export FBX → UE5 retarget | |
| **Hit Impact VFX** | Particle spray when weapon hits Guardian | UE5 Niagara built-in "Sparks" or "Blood" template | No purchase needed |

---

### Before P3.8 — The Monolith (World Hero Asset)

The central structure of the entire game world. Every screenshot, every gameplay moment references this structure.

| Asset | Description | Source | Priority |
|---|---|---|---|
| **The Monolith** (Static Mesh) | The large, imposing central obelisk/tower at the world's center. Vertical, mysterious, stylized. Should convey power and scale. Bands of altitude wrap around it | **Custom model in Blender** — the project plan explicitly states "these two things (the Monolith and Guardian) are what every screenshot and demo clip will focus on — leverage your advanced Blender skills here" | 🔴 Critical — Model this yourself |
| **Monolith Emissive Material** | Glowing energy veins running along the Monolith surface | UE5 Material Editor — procedural emissive mask using world-position offset. No asset purchase needed | |

> **Blender Tip for the Monolith:** Start with a tall narrow cylinder. Add stone panel bevels. Use geometry nodes or manual edge cuts to carve energy-line channels. Apply a dark stone base material with an emissive orange/teal mask in UE5. Export as FBX with UV maps.

---

### Before P3.9 — Checkpoint Respawn Actors

Physical checkpoint markers placed in the world that serve as respawn anchors.

| Asset | Description | Source |
|---|---|---|
| **Checkpoint Gate / Platform** (Static Mesh) | A glowing platform or gate arch that marks each respawn point and the share-gated altitude barrier | **Option A:** Assemble from modular Fab ruins pieces. **Option B:** Simple geometry in UE5 BSP + emissive material. **Option C:** Fab: `"energy gate UE5"` or `"stylized checkpoint"` |
| **Gate Open/Close VFX** | Particle dissolve or energy field effect when gate unlocks | UE5 Niagara: use a "ribbon" or "mesh emitter" on the gate collision volume |

---

### P3.11 — Full Art Pass on Level 1 (The Big One)

This is where all environment assets come together simultaneously.

#### Environment Fill (Quixel Megascans — All Free)

Open **UE5 → Fab (Bridge) plugin** and download these Megascans packs directly:

| Category | Megascans Search Term | Use |
|---|---|---|
| Rock & Cliff Terrain | `"Rocky Cliff Formation"`, `"Limestone"` | Altitude band walls and terrain |
| Ground Surface | `"Cracked Stone Ground"`, `"Mossy Rock"` | Band floor surfaces |
| Ruins Architecture | `"Ancient Stone Pillar"`, `"Broken Archway"` | Decorative structural elements |
| Foliage / Vegetation | `"Fern"`, `"Mountain Grass"`, `"Floating Moss"` | Organic fill between rock formations |
| Sand / Dust | `"Fine Sand"`, `"Volcanic Ash"` | Ground variation on lower bands |

#### Atmosphere & Lighting (All Built-In — Free)

| Element | UE5 Tool | Notes |
|---|---|---|
| Sky | **Sky Atmosphere** component | Already in engine — no purchase |
| Clouds | **Volumetric Clouds** | Already in engine |
| Sunlight | **Directional Light** + **Sky Light** | Configure per-band mood via sky material |
| Dynamic GI | **Lumen** | Enabled by default in UE5 — handles reflections automatically |

#### VFX Requirements (All Niagara — Built-In — Free)

| Effect | Trigger | UE5 Niagara Template to Start From |
|---|---|---|
| Jetpack Thrust | While `GA_Jetpack` is active | `"Sprite Burst"` → modify to fire/smoke look |
| Weapon Muzzle Flash | On `GA_FireWeapon` activation | `"Sprite Burst Instant"` |
| Weapon Hit Impact | On hit-scan contact point | `"Sparks Impact"` |
| Golden Apple Share Effect | On successful share event | `"Glow Pulse"` sphere emitter |
| Death Dissolution | On player/guardian death at `Health <= 0` | `"Mesh Emitter Dissolve"` or simple screen fade |
| Checkpoint Unlock | When gate opens for a player | `"Ring Burst"` or `"Energy Wave"` |

---

## 🏪 Recommended Fab Search Queries

Navigate to **fab.com** and use these exact searches. Always filter by:
- ✅ **Compatible with UE 5.x**
- ✅ **Standard License** (allows commercial use)

| Search Query | What You're Looking For |
|---|---|
| `stylized character UE5 free standard license` | Player character skeletal mesh |
| `modular ruins environment stylized` | Level 1 architecture filler |
| `sci-fi stylized weapon pack free` | Primary ranged weapon mesh |
| `energy barrier gate UE5` | Checkpoint gate mesh |
| `jetpack prop backpack UE5` | Jetpack attachment mesh |
| `stylized fantasy props pack` | Item props (apple, blossom, etc.) |

---

## ✅ Summary: Custom Blender Models Required

These two assets are specifically called out as critical custom work:

| Asset | Why Custom | Estimated Blender Time |
|---|---|---|
| **The Monolith** | Central to every screenshot, marketing image, and demo clip | 4–8 hours |
| **The Guardian Character** | Primary enemy, appears constantly in gameplay footage | 6–12 hours |

Everything else can legitimately be sourced from **Quixel Megascans** (free) or **Fab Standard License** assets, keeping the project within a realistic solo development timeline.

---

## 📦 Git LFS Note

Art assets (`.fbx`, `.uasset` with large texture maps) can push a Git repository to unusable sizes. If any single texture or mesh exceeds **50 MB**, set up Git LFS before committing:

```bash
git lfs install
git lfs track "*.fbx"
git lfs track "*.uasset"
git add .gitattributes
git commit -m "chore: configure Git LFS for large art assets"
```

Document this in `Docs/ArtPipeline.md` once Level 1 art pass is complete.
