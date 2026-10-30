# EchoVault — a time-loop puzzle game in C++17

**Outsmart doors with your own recorded past.** EchoVault is a three-level time-loop puzzle game powered by a deterministic C++17 engine. Every rewind creates an echo that replays your previous movements; use those copies to hold pressure plates and cross otherwise inaccessible gates.

**[▶ Play EchoVault in your browser](https://ghassen258.github.io/echovault/)** — WebAssembly build, no installation required.

## Features

- Three handcrafted levels, culminating in a three-gate time-loop puzzle
- Replayable movement recordings that persist at their final location
- Pressure plates, doors, and timing-based cooperative puzzles with past selves
- Browser version compiled from the original C++ game engine with WebAssembly, with animated canvas rendering and touch controls
- Native terminal version with ANSI colors, immediate WASD/arrow-key input, and rewind animation
- Portable C++17 game engine with zero third-party dependencies
- Scripted command mode for reproducible runs and CI testing
- Automated CMake/CTest tests on Linux, macOS, and Windows

## Play in the browser

Open **[EchoVault — Browser Edition](https://ghassen258.github.io/echovault/)**. All three levels run directly in the browser; the C++ engine is compiled to WebAssembly by Emscripten. No account, server-side gameplay, or installation is required.

Use arrow keys or WASD to move, **R** to create an echo, **Space** or **.** to wait, **C** to restart, and **N** to advance after clearing a room. On touchscreens, use the on-screen directional pad and action buttons.

## Run the native C++ edition

**Requirements:** a C++17 compiler and CMake 3.16 or later. A modern terminal is recommended for colors and single-key input.

```bash
cmake -S . -B build
cmake --build build --config Release
```

Run on Linux/macOS:

```bash
./build/echovault
```

Run on Windows (PowerShell, Visual Studio CMake generator):

```powershell
.\build\Release\echovault.exe
```

Depending on the generator, Windows may place the executable at `build\echovault.exe` instead. Alternatively, compile directly with a C++17 compiler:

```bash
g++ -std=c++17 -O2 -Isrc src/game.cpp src/main.cpp -o echovault
./echovault
```

**Animated demonstration:** run `echovault --demo` (or `./build/echovault --demo`) to watch a complete solution to the first level.

## Controls

| Key | Action |
|---|---|
| **W A S D** or arrow keys | Move one tile |
| **.** or **Space** (browser) | Wait one tick to synchronize with an echo |
| **R** | Rewind to the start, recording your current route as a new echo |
| **C** | Reset the level and erase all echoes |
| **H** | Show or hide gameplay instructions |
| **N** | Advance after clearing a level |
| **Q** | Quit |

Each movement or wait advances time by exactly one tick. A blocked move also advances the clock. An echo replays one recorded position per tick and remains on its last tile after its route ends. The player can create up to eight echoes per level.

### Symbol legend

`@>` player · `1*` first echo · `a_` pressure plate · `AX` locked gate · `A/` open gate · `<>` exit · `##` wall

Switch `a` controls door `A`, `b` controls `B`, and so on. A gate remains open while the player or any echo occupies its matching plate. Crucially, to walk *off* a plate and through its gate, a recorded echo must hold that plate in the new timeline.

## How the game works

1. **Record** a route ending on a pressure plate.
2. **Rewind** (`R`) to create an echo of that route.
3. **Synchronize** your movement with the echo while it replays its steps.
4. **Cross** the opened gate and repeat for additional plates.

The later levels require multiple independent recordings to cooperate. Rewinding never changes an earlier recording; each echo remains immutable after creation.

## Code architecture

| Path | Responsibility |
|---|---|
| `src/game.hpp` | Public state models and game API |
| `src/game.cpp` | Deterministic game logic, collision detection, door rules, echo playback, level generation |
| `src/main.cpp` | Cross-platform keyboard handling, ANSI terminal rendering, rewind effects, command-line interface |
| `src/web_bridge.cpp` | Emscripten bindings exposing the same C++ engine to JavaScript as JSON snapshots and commands |
| `web/index.html`, `web/styles.css`, `web/game.js` | Browser interface, responsive controls, visual feedback, Canvas 2D renderer |
| `.github/workflows/pages.yml` | Compiles C++ to WebAssembly and publishes the browser build |
| `tests/test_game.cpp` | End-to-end game-logic scenarios, including scripted victories in all three levels |
| `.github/workflows/tests.yml` | Three-OS build and test matrix |

The game engine has no dependency on terminal I/O, making it possible to test game rules independently of rendering. Both the terminal executable and browser game call **the same** `Game::command()` implementation—there is no duplicated JavaScript version of the puzzle rules. The implementation uses value semantics (`std::vector<Point>` recordings), explicit game state transitions, and deterministic replay.

### Build the browser edition locally

The browser build requires the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) and a local HTTP server. With the SDK environment activated:

```bash
mkdir -p dist
em++ -std=c++17 -O2 -Isrc src/game.cpp src/web_bridge.cpp \
  --bind -s NO_ENTRY=1 -s MODULARIZE=1 -s EXPORT_ES6=1 -s ENVIRONMENT=web \
  -s FILESYSTEM=0 -s ALLOW_MEMORY_GROWTH=1 -s ASSERTIONS=0 \
  -o dist/echovault.mjs
cp web/index.html web/styles.css web/game.js dist/
python -m http.server 8000 --directory dist
```

Open `http://localhost:8000`. The compiler emits `echovault.mjs` and `echovault.wasm` into `dist/`; this directory is generated and isn't committed. Run the native CMake tests to validate gameplay rules independently of the browser presentation.

## Tests

```bash
ctest --test-dir build -C Release --output-on-failure
```

For headless/scripted usage, for example:

```bash
./build/echovault --no-color --script ddssssrssdddddddddddddd
```

This example records a short first echo and solves the first vault. The `--script` argument does not require interactive input or ANSI terminal features.

## Technical notes

- Echoes are time-indexed position recordings, not AI-controlled actors.
- Locked doors check occupied pressure plates at the next simulation tick.
- The game deliberately uses turn-based time rather than frame-dependent physics, so a run is reproducible on different machines.
- There is no save file or external service; each launch begins a new run. The browser build uses the same rules and level data as the native version.

## License

MIT © 2026 Ghassen258.
