# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

vibe is a set of C++17 engine libraries (audio, graphics, fluid sim, local-multiplayer game layer) built on Magnum/OpenGL, ImGui and libsoundio. Apps and games live in other repos and build against this one.

## Layout

```
engine/common    Vibe::Common    logging, errors, Armadillo typedefs
engine/audio     Vibe::Audio     LibSoundIo I/O, WAV/FFmpeg decode, FFT, pitch, synthesis
engine/graphics  Vibe::Graphics  Magnum/OpenGL rendering, ImGui, hitboxes, meshes, MIDI parsing, Camera2D/Batch2D, OrbitCamera
engine/fluid     Vibe::Fluid     2D stable-fluids solver, audio emitters, BandTracker (headless; PIC)
                 Vibe::FluidPresets  strict JSON presets (jsoncpp; executables only)
engine/game      Vibe::GameCore  input (ActionMap/InputSystem), Lobby, ViewLayout, FixedStepper, InputScript (headless)
                 Vibe::Game      SdlInput, Hud, GameApp (smg::GuiBase app shell)
cmake/           VibeDeps.cmake (consumer search paths), VibeConfig.cmake.in (find_package(Vibe)), VibeManifest.cmake
deps/            third-party superbuild -> .deps/usr
tests/           older engine tests (armadillo, hitbox, audio pipeline, ...)
external/        submodules: simple-magnum-gui (smg), libsoundio
```

**Boundary rule:** engine code never includes from apps or games, and never hardcodes paths outside the repo.

Headers are included as `vibe/<lib>/X.hxx`. In the build tree that spelling resolves through symlinks in `build/include/vibe/`, and in an install it resolves through `include/vibe/`.

## Build Commands

CMake 3.24+ and Ninja. Two stages: third-party deps once, then the engine.

Keep builds niced and capped (`nice -n 19`, `--parallel 4`). The Makefile defaults to `--parallel 14`, so pass `PARALLEL="--parallel 4"`.

```bash
nice -n 19 make deps PARALLEL="--parallel 4"     # third-party deps -> .deps/usr (slow, rarely needed)
cmake --preset linux
nice -n 19 cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
cmake --install build --prefix <prefix>          # engine libs, headers, Vibe package configs, deps libs
```

`make distclean` deletes `.deps/`, which is a long rebuild. To clear only the engine build, use `rm -rf build`.

## Two ways consumers use the engine

- **Subproject** (side-by-side development): `include(<vibe>/cmake/VibeDeps.cmake)` then `add_subdirectory(<vibe> vibe)`. The top-level `CMakeLists.txt` checks `PROJECT_IS_TOP_LEVEL` and, as a subproject, builds the engine libs only (no tests). Anything that only makes sense at the top level goes after its `return()`.
- **Installed**: `find_package(Vibe CONFIG REQUIRED)` with `CMAKE_PREFIX_PATH="<prefix>;<vibe>/.deps/usr"`.

Keep both working: use `CMAKE_CURRENT_SOURCE_DIR`/`PROJECT_SOURCE_DIR`/`DEPS_PREFIX`, never `CMAKE_SOURCE_DIR` (it is the consumer's root in subproject mode). The one deliberate exception is `VIBE_ASSETS_DIR` in `engine/game`, which defaults to the consumer's `assets/`.

## Code Quality

C++ style is pinned by `.clang-format`, `.clang-tidy` and `.clangd`.

## Key Dependencies

Bundled (built in `.deps/`): Armadillo, SuperLU, jsoncpp, SMG (Magnum GUI wrapper), LibSoundIo, Magnum/Corrade

System: Boost 1.80+, TCLAP, OpenGL, SDL2, FFmpeg, FFTW, ncurses (only with `-DENABLE_CURSES=ON`)
