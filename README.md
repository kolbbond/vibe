# vibe

C++17 engine libraries for music-driven graphics and small local-multiplayer
games, built on [Magnum](https://magnum.graphics), ImGui and libsoundio.

| Library | Target | What it does |
|---|---|---|
| `engine/common` | `Vibe::Common` | logging, errors, Armadillo typedefs |
| `engine/audio` | `Vibe::Audio` | libsoundio I/O, WAV/FFmpeg decode, FFT, pitch, synthesis |
| `engine/graphics` | `Vibe::Graphics` | Magnum/OpenGL rendering, ImGui, meshes, cameras, 2D batching |
| `engine/fluid` | `Vibe::Fluid`, `Vibe::FluidPresets` | 2D stable-fluids solver, audio emitters, JSON presets |
| `engine/game` | `Vibe::GameCore`, `Vibe::Game` | input actions, lobby, split/shared views, fixed-step loop, app shell |

Apps built on it: [vibe-apps](https://github.com/kolbbond/vibe-apps) (the
vvavviz music visualizer).

## Build (Linux)

Needs CMake 3.24+, Ninja, a C++17 compiler, and system Boost 1.80+, TCLAP,
OpenGL, SDL2, FFmpeg and FFTW. Everything else is built by the `deps/`
superbuild into `.deps/usr`.

```bash
git clone --recurse-submodules https://github.com/kolbbond/vibe.git
cd vibe
make deps                     # third-party libraries -> .deps/usr (slow, once)
cmake --preset linux
cmake --build build
ctest --test-dir build --output-on-failure
```

## Using it from another project

**Side by side** (engine edits rebuild your project): check `vibe` out next to
your project and, in your top-level `CMakeLists.txt`:

```cmake
set(VIBE_ROOT "${CMAKE_SOURCE_DIR}/../vibe" CACHE PATH "vibe engine checkout")
include(${VIBE_ROOT}/cmake/VibeDeps.cmake)   # .deps/usr search paths and RPATH
add_subdirectory(${VIBE_ROOT} vibe)          # engine libs only, no engine tests
target_link_libraries(myapp PRIVATE Vibe::Game)
```

**Installed:** `cmake --install build --prefix <prefix>`, then in your project
`find_package(Vibe CONFIG REQUIRED)` with
`-DCMAKE_PREFIX_PATH="<prefix>;<vibe>/.deps/usr"`.

Either way `include(VibeManifest)` gives `vibe_add_manifest()` /
`vibe_write_manifests()`, which collect `game.json` files into
`build/games.json` for a launcher.

## License

MIT, see [LICENSE](LICENSE).

Bundled third-party code keeps its own license: `engine/audio/include/AudioFile.hxx` is [AudioFile](https://github.com/adamstark/AudioFile) 1.1.1 by Adam Stark (MIT, notice in the file), and the submodules under `external/` are MIT: [libsoundio](https://github.com/kolbbond/libsoundio) (a fork of Andrew Kelley's) and [simple-magnum-gui](https://github.com/kolbbond/simple-magnum-gui), whose bundled fonts (SIL OFL 1.1 and Apache 2.0) are listed in its `assets/LICENSES`.
