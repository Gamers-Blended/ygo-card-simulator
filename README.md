# ygo-card-simulator

[![CI](https://github.com/Gamers-Blended/ygo-card-simulator/actions/workflows/ci.yml/badge.svg)](https://github.com/Gamers-Blended/ygo-card-simulator/actions/workflows/ci.yml)


Notes

Stage 0
- `vcpkg.json` lists what the project needs.
- `builtin-baseline` pins which versions. It's a commit of the vcpkg registry, and every dependency resolves to the version recorded at that commit.
- Without the pin, a "classic mode" global install would drift over time.
- The libraries install per project into the build folder, which is why `vcpkg_installed/` is already in `.gitignore`.
- Ubuntu's desktop session reads environment variables from: `mkdir -p ~/.config/environment.d && echo 'VCPKG_ROOT=$HOME/vcpkg' > ~/.config/environment.d/vcpkg.conf`
- Log out and back in.
- Reconfigure Qt Creator's build folder: **Build** → **Clear CMake Configuration**, **Run CMake**

Stage 1
- To try demo: `~/Qt/Tools/CMake/bin/cmake --build build/cli && ./build/cli/odds_demo`
