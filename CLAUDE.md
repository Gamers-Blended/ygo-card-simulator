# CLAUDE.md

Yu-Gi-Oh! booster pack simulator: Qt 6 / QML, C++20, CMake. The full plan lives in
[`docs/ygo-booster-sim-design.md`](docs/ygo-booster-sim-design.md). Read it before starting work on a stage.
It's the source of truth for features, architecture, the odds model (Section 7) and the roadmap (Section 10).

## This is a learning project

The owner is learning C++ through this repo, and understanding matters more than speed.
- Work in small steps that build and run. Don't write a whole stage in one go.
- Explain new C++ ideas (RAII, smart pointers, move semantics, templates and so on) the first time they appear, and explain why the code is shaped the way it is.
- Prefer clear, idiomatic code over clever code.

## Workflow

- One branch per roadmap stage (`stage-N-<name>`), merged into `main` through a PR. Don't commit to `main` directly.
- Only commit or push when asked. The owner usually commits from Qt Creator's Git UI.
- Commit message style: `type: summary` (e.g. `refactor: ...`, `config: ...`, `feat: ...`).
- When a decision changes, update the design doc in the same branch.

## Architecture rules (see design doc Sections 4–6)

- The UI is **pure QML** (Qt Quick + Qt Quick Controls). No Qt Widgets.
- `src/odds`, `src/catalog` and `src/persistence` must have **no Qt dependency**, so they can be unit-tested without a UI.
- The network is only used by the Sync Service. Follow the ygoprodeck rules in Section 2: cache data locally, never hotlink images, and stay under 20 requests/second.
- Per-set odds are data (`data/*.json`), not code.

## Layout

- `src/app/main.cpp`: entry point that loads the QML module `ygoCardSimulator`
- `qml/`: QML files, registered with `qt_add_qml_module` in `CMakeLists.txt`
- `docs/`: design doc
- New source and QML files must be added to `CMakeLists.txt`.

## Build and run (local Linux setup)

`cmake` isn't on PATH. Use Qt's bundled tools. Use `build/cli` so the build doesn't clash with Qt Creator's `build/Desktop_Qt_*` folder.

```sh
~/Qt/Tools/CMake/bin/cmake -S . -B build/cli -G Ninja \
  -DCMAKE_MAKE_PROGRAM=$HOME/Qt/Tools/Ninja/ninja \
  -DCMAKE_PREFIX_PATH=$HOME/Qt/6.12.0/gcc_64
~/Qt/Tools/CMake/bin/cmake --build build/cli
./build/cli/appygoCardSimulator
```

For a headless smoke test, run `QT_QPA_PLATFORM=offscreen timeout 3 ./build/cli/appygoCardSimulator`. Exit code 124 means it was still running when the timeout stopped it, so QML loaded. A quick `-1` exit means QML failed to load.
