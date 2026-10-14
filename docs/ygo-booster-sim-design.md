# Yu-Gi-Oh! Booster Pack Simulator — Design Document

## 1. Overview

A local, cross-platform (Windows + Linux) C++ desktop application that simulates opening Yu-Gi-Oh! booster packs using real card data and rarity odds pulled from [ygoprodeck.com](https://ygoprodeck.com), with a binder/collection system and animated pack-opening sequence.

**Project purpose:** this is a learning project. The primary goal is to gain hands-on C++ experience (coming from little/no prior C++ background), built incrementally, milestone by milestone, in a public GitHub repo, using Claude Code as a step-by-step development partner. Feature completeness matters less than building understanding at each stage — the roadmap in Section 10 is written with that in mind.

## 2. Compliance Note (Read First)

ygoprodeck's API guide requires:
- Downloading and storing all API data locally — do not call the API on every action.
- Never hotlinking images — download and host them yourself.
- Respecting the 20 requests/second rate limit (a 1-hour IP block follows violations).

**Design implication:** the app performs an explicit "Sync Card Database" action (on first run / on demand), caches everything locally, and is fully offline-capable afterward. The network layer never sits in the hot path of opening a pack.

## 3. Feature-to-System Mapping

| # | Feature | Owning System |
|---|---|---|
| 1 | Pull images/info from ygoprodeck | Sync Service (network layer) |
| 2 | Pack odds matching real rarity distribution | Odds Engine |
| 3 | Save/load to external `.sav` | Persistence Layer |
| 4 | Binder, 9 cards/page, unlimited pages, paged flip-through | Collection/Binder System + QML UI |
| 5 | Pack-opening animation + sparkle VFX/SFX | QML Render/Animation Layer |
| 6 | Autosave on each opening | Persistence Layer |
| 7 | Add new cards to binder | Collection/Binder System |
| 8 | Update card count in collection | Collection/Binder System |

## 4. Decisions Locked In

| Decision | Choice | Notes |
|---|---|---|
| GUI framework | **Qt/QML (pure QML, no Widgets)** | Whole UI, including the app shell, is Qt Quick + Qt Quick Controls. C++ backend exposed to QML via `QObject`-derived models/context properties; QML handles declarative animation (flip, slide, particles) |
| Binder UI | **Paged flip-through** (not infinite scroll grid) | Each page = 9 cards (3x3), page-turn transition between pages |
| Per-set odds tables | **User-supplied incrementally** | Odds Engine reads from a config file/table per set; new sets can be added without code changes — see Section 7 |
| Distribution baseline | Portable zip (Windows) + AppImage (Linux) | Installer/package manager entries are a later, optional polish step |
| Qt linking | **Dynamic linking** | Avoids LGPL static-linking obligations; use `windeployqt` / `linuxdeployqt` to bundle Qt libs alongside the exe |
| Small dependency linking | **Static** (nlohmann/json, SQLite, miniaudio, cpr) | These are header-only or permissively licensed; static linking simplifies distribution with no licensing complications |
| Dependency management | **vcpkg manifest mode** (`vcpkg.json`) | Reproducible, isolated per-project installs; single source of truth for local + CI builds — avoids "classic mode" version drift between machines |
| Sparkle VFX | **QML `ParticleSystem`** | Native QtQuick particle module, no custom shader needed for stage 7 |
| Save file location | **OS-standard convention** — see Section 8 | Matches user expectations on each platform; avoids polluting install directory or home folder root |

## 5. Recommended Stack

| Layer | Choice | Why |
|---|---|---|
| Language/Build | C++20, CMake + vcpkg (manifest mode) | Reproducible cross-platform builds via `vcpkg.json`; standard for a Qt+CMake project |
| GUI/Animation | **Qt 6 Quick (QML) + Qt Quick Controls for the whole UI** | `ApplicationWindow`, `StackView`, `Dialog` and `QtQuick.Dialogs` cover the shell; `Behavior`, `Transition`, `State`, and `ParticleSystem` map directly to flip/slide/sparkle requirements. One UI technology means no Widgets/QML embedding (`QQuickWidget` render overhead, focus/stacking issues) and one less Qt module to deploy |
| Networking | cpr (libcurl wrapper) | Ergonomic, used only by Sync Service |
| JSON | nlohmann/json | Simple, sufficient for small local payloads |
| Persistence | SQLite (catalog + collection) + JSON (settings) | Transactional safety, queryable collection |
| Audio | miniaudio | Single-header, no extra system deps, easy to call from C++ backend on flip/reveal events |
| Testing | GoogleTest / Qt Test | Unit-test Odds Engine + Persistence deterministically, independent of QML |
| CI | GitHub Actions (windows-latest, ubuntu-latest matrix) | Catch platform issues per commit; also a good early C++ learning exercise (build systems, CI config) |

## 6. Module Breakdown

```
/src
  /sync          -> HTTP calls, JSON parsing, image download/cache, rate limiting
  /catalog       -> Local card database (SQLite), set/rarity metadata
  /odds          -> Pure, engine-agnostic pack odds simulation (unit-testable, no Qt dependency)
  /collection    -> Binder model exposed to QML (QAbstractListModel), card counts, pagination
  /persistence   -> .sav read/write, atomic writes, schema versioning
  /audio         -> SFX playback (foil reveal, card flip), triggered from C++ on QML signals
  /app           -> Main.cpp, QQmlApplicationEngine setup, context property registration
/qml
  Main.qml            -> App shell / navigation
  BinderPage.qml       -> 3x3 grid, one binder page
  BinderView.qml       -> Paged flip-through container (PathView or SwipeView)
  PackOpening.qml      -> Card-back fan-out, flip sequence, sparkle ParticleSystem
  CardDelegate.qml     -> Single card visual (front/back, flip Behavior)
```

### Dependency direction
QML views bind to C++ models/objects exposed via `qmlRegisterType` or context properties. `odds`, `catalog`, and `persistence` have **zero** dependency on Qt/QML and can be fully unit-tested headlessly — this separation is also good practice to learn early, since it keeps business logic testable independent of UI framework choice.

## 7. Odds Engine Design

**Model:** hybrid slot-based system, driven by a per-set config (data-driven, not hardcoded), so odds tables can be added/updated without recompiling:

```json
{
  "set_code": "LOB",
  "total_cards_per_pack": 9,
  "filler_slots": 8,
  "filler_pool": "Common",
  "guarantee_slots": [
    {
      "slot_index": 9,
      "table": [
        { "rarity": "Rare",  "weight": 70 },
        { "rarity": "Super", "weight": 20 },
        { "rarity": "Ultra", "weight": 10 }
      ]
    }
  ]
}
```

Odds tables will be supplied incrementally per set — the engine should be written against this schema from the start so new sets are a data change, not a code change.

**Foil is a property of rarity, not a separate roll.** There is no per-slot foil chance. Whether a pulled card is foil comes from its rarity (in LOB: Super, Ultra and Secret Rare are foil; Rare and Common are not). Since only the guarantee slot can roll a foil rarity, "at most one foil per pack" falls out of the slot model with no extra rule. If a later set has foils that replace a filler card, model that as another slot with its own table, which is still a data change.

### Set: Legend of Blue Eyes White Dragon (LOB)

Supplied odds (converted to per-slot probabilities for the guarantee slot):

| Rarity | Advertised odds | Per-pack probability | Notes |
|---|---|---|---|
| Super Rare | 1:6 packs | 1/6 ≈ 16.67% | |
| Ultra Rare | 1:12 packs | 1/12 ≈ 8.33% | |
| Secret Rare | 1:36 packs | 1/36 ≈ 2.78% | |
| Rare | (remainder) | 1 − (1/6 + 1/12 + 1/36) = 26/36 ≈ 72.22% | Fills the guarantee slot when none of the above hit |

```json
{
  "set_code": "LOB",
  "total_cards_per_pack": 9,
  "filler_slots": 8,
  "filler_pool": "Common",
  "guarantee_slots": [
    {
      "slot_index": 9,
      "table": [
        { "rarity": "Secret Rare", "odds_denominator": 36 },
        { "rarity": "Ultra Rare",  "odds_denominator": 12 },
        { "rarity": "Super Rare",  "odds_denominator": 6 },
        { "rarity": "Rare",        "remainder": true }
      ]
    }
  ]
}
```

**Implementation note:** advertised "1 in N packs" odds are the *overall* chance of seeing that rarity in a pack, so the engine uses each `1/N` directly as that rarity's probability. The entry marked `"remainder": true` gets whatever probability is left over (exactly one entry per table may be the remainder). Then **one uniform roll** in `[0, 1)` picks the rarity by walking the cumulative probabilities.

Do **not** model this as sequential "roll down from rarest" checks (roll 1/36 for Secret, on a miss roll 1/12 for Ultra, and so on). Each later roll only happens when the earlier ones missed, so it under-delivers every rarity after the first: Ultra comes out at (35/36)·(1/12) ≈ 8.10% (≈1:12.3) instead of 8.33%, and Super at ≈ 14.85% (≈1:6.7) instead of 16.67%.

Weighted tables (like the generic example above) normalize to probabilities the same way (`weight / sum of weights`), so after loading, both config styles become the same internal probability table and use the same single-roll code path. At load time, validate that the explicit probabilities sum to at most 1, and reject the config otherwise.

**Key edge case:** seed `std::mt19937` once per session (or persist RNG state) — do not reseed per pack, or rapid "open all" clicks with a time-based seed can produce correlated results.

## 8. Persistence Design

- **Card catalog cache:** SQLite table, keyed by card ID + image URL hash (handles ygoprodeck art updates without orphaning cached images).
- **User collection:** SQLite table `owned_cards(card_id, count, first_acquired_at)` — feature 8 (update count) becomes a single `UPDATE ... SET count = count + 1`.
- **Save file (`.sav`):** wraps SQLite state + a schema version field, written via **write-to-temp-then-atomic-rename** on every autosave to prevent corruption on crash/power loss.
- **Versioning:** schema version present from v1, with a migration path for future field additions.
- **Save file location (OS-standard convention):**
  - **Windows:** `%APPDATA%\YgoBoosterSim\` (i.e. `C:\Users\<user>\AppData\Roaming\YgoBoosterSim\`) — standard per-user application data location on Windows.
  - **Linux:** `$XDG_DATA_HOME/ygo-booster-sim/`, falling back to `~/.local/share/ygo-booster-sim/` when `XDG_DATA_HOME` is unset — the XDG Base Directory spec convention followed by most modern Linux apps.
  - Both cases: create the directory on first run if it doesn't exist; store the `.sav` file and the local SQLite card-catalog cache in this same directory (separate files, not one blob) so a corrupted save doesn't require re-syncing the entire card database.

## 9. Animation Sequence (Feature 5, QML-specific)

1. Pack asset appears; on tap/click, 9 `CardDelegate` items animate out from the pack using a `Behavior on x/y` or `ParallelAnimation`, fanning into position.
2. Cards flip face-up one at a time (sequenced via `SequentialAnimation` + `PauseAnimation` between each), using a `RotationAnimation` on the `y` axis with a texture swap at 90°.
3. On flip completion, if the revealed card's rarity is a foil rarity (e.g. Super/Ultra/Secret Rare): trigger a QML `ParticleSystem` (sparkle) + emit a signal to C++ to play the chime SFX via miniaudio.
4. On full reveal, C++ backend triggers autosave (feature 6) before returning to binder/menu.
5. Binder page turns (`BinderView.qml`) use a `PathView` or `SwipeView` with a page-curl-style `Transition` between pages.

## 10. Learning Roadmap (Step-by-Step Milestones)

Written as incremental, independently-committable stages — each stage should end with something that builds, runs, and can be pushed to GitHub before moving to the next. Suggested as a rough order; adjust pace as comfort with C++ grows.

| Stage | Goal | New C++/tooling concepts introduced |
|---|---|---|
| 0 | Repo scaffold: CMake "hello world" Qt Quick window, GitHub repo + Actions CI building on Windows & Linux | CMake basics, Qt/QML project structure, CI config |
| 1 | Odds Engine as standalone library + GoogleTest unit tests, no UI at all (console print of simulated pulls) | Classes, structs, `std::vector`, RNG (`<random>`), unit testing |
| 2 | Sync Service: fetch + cache one set's card data + images from ygoprodeck to local SQLite/disk | HTTP client usage, JSON parsing, file I/O, error handling |
| 3 | Collection/Persistence layer: SQLite schema, add/query owned cards, atomic save file writes | RAII, smart pointers, exception safety, SQLite C++ wrapper |
| 4 | Basic QML binder view (static, non-paged) rendering cached card images from the collection model | Qt model/view (`QAbstractListModel`), C++/QML interop |
| 5 | Paged flip-through binder UI | `PathView`/`SwipeView`, QML transitions |
| 6 | Pack-opening animation (no VFX/SFX yet) — cards slide out and flip | QML `SequentialAnimation`, signals/slots between QML and C++ |
| 7 | Sparkle VFX + SFX on foil reveal | QML `ParticleSystem`, miniaudio integration |
| 8 | Autosave wiring + full save/load round-trip testing | Integration testing, schema versioning in practice |
| 9 | Polish pass: multiple sets, incremental odds tables added, packaging (portable zip / AppImage) | Deployment tooling (`windeployqt`, AppImage), packaging |

## 11. Suggested Repo & Workflow

```
/repo-root
  CMakeLists.txt
  /src, /qml         (see Section 6)
  /tests             -> GoogleTest suites (odds, persistence)
  /data              -> per-set odds config JSON files (added incrementally)
  /docs              -> this design doc, ADRs (Architecture Decision Records) as decisions evolve
  .github/workflows/ -> CI build matrix
  .gitignore         -> build/, vcpkg_installed/, *.sav (user save files), *.db (local SQLite catalog cache) — keep both out of the repo
  README.md          -> build instructions, project status/roadmap checklist
```

**Working with Claude Code, stage by stage:**
- One feature branch per roadmap stage (`stage-1-odds-engine`, `stage-2-sync-service`, etc.), merged to `main` via PR once it builds + tests pass — good practice to build even solo, and keeps history readable.
- Ask Claude Code to explain unfamiliar C++ idioms as they're introduced (smart pointers, RAII, move semantics) rather than just accepting generated code — since the goal here is learning, not just shipping.
- Commit early and often within a stage; small, working commits make it easy to roll back if a Claude Code-driven change introduces something you don't understand yet.
- Keep Section 10's stage boundaries as PR boundaries — each PR should be independently buildable and testable, which also keeps CI meaningful from stage 0 onward.

## 12. Open Questions / Next Steps

- [x] Sparkle VFX approach — QML `ParticleSystem` confirmed.
- [x] Dependency management mode — vcpkg manifest mode confirmed.
- [x] First per-set odds table — LOB provided (Section 7); validates the "1 in N + remainder" schema pattern for future sets.
- [x] Save file location convention — OS-standard paths confirmed (Section 8).
- [ ] Additional set odds tables to be supplied incrementally as they're provided — each new set should reuse the LOB schema pattern (weighted table *or* "1 in N" odds with a remainder entry, whichever matches the source odds; both resolve to one probability table and a single roll).
- [x] `.gitignore` excludes the local SQLite catalog cache (`*.db`) in addition to `*.sav` — it's regenerable via Sync Service and shouldn't be committed.

---
*This document reflects design decisions made through planning; treat as a living reference — update it as stages are completed and decisions evolve.*
