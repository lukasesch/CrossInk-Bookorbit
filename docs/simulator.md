---
title: Simulator
nav_order: 15
---

# Development Device Simulator

CrossInk can run in the [CrossPoint simulator](https://github.com/uxjulia/crosspoint-simulator), which renders the e-ink display in an SDL2 window. Use it for quick sanity checks without flashing firmware every time.

## Platform Support

The simulator is currently configured for macOS on Apple Silicon.

The `platformio.ini` `[env:simulator]` section contains hardcoded `-arch arm64` and Homebrew paths under `/opt/homebrew`.

- Intel Mac users need to remove `-arch arm64` and change Homebrew paths to `/usr/local`.
- Linux requires similar path changes plus a replacement for `lib/simulator_mock/src/MD5Builder.h`, which uses the macOS-only `CommonCrypto` API.
- Native Windows is not supported. Use WSL and follow the Linux adjustments.

## Prerequisites

```sh
# macOS
brew install sdl2

# Linux (Debian/Ubuntu)
sudo apt install libsdl2-dev
```

## Setup

Place EPUB books in `./fs_/books/` relative to the project root. That maps to the SD-card `/books/` path on device.

## Build And Run

```sh
pio run -e simulator
.pio/build/simulator/program
```

Use the X4 Pro environment to enable its touch, frontlight, and Home-key behavior:

```sh
pio run -e x4-pro-simulator -t run_simulator
```

## Keyboard Controls

| Key | Action |
| --- | --- |
| Up / Down | Page back / forward (side buttons) |
| Left / Right | Left / right front buttons |
| Return | Confirm / Select |
| Escape | Back |
| P | Power |
| H | X4 Pro Home key (tap to go Home; hold for 700 ms to toggle the reader menu) |

The `H` mapping is active only in `x4-pro-simulator`.

## BOOX Picco Simulator

`picco-simulator` runs the simulator's X4 Pro profile (480x800 portrait, touch, frontlight with warmth) as a Picco: no Home key, the Picco's global touch lock, and `CROSSINK_APP_DEVICE_PICCO` / `CROSSINK_APP_PICCO_UI` for app code. `L` toggles a stand-in slide switch (`CROSSPOINT_SIM_SLIDE_SWITCH=on` starts it on). The fetched simulator package needs the repo's shims once per env:

```sh
pio pkg install -e picco-simulator
python3 test/integration/ci/patch_simulator_package.py picco-simulator
pio run -e picco-simulator -t run_simulator
```

### Screenshots of every screen

The visual gate for the Picco UI work writes one PNG per reachable screen, named after it (`home.png`, `reader.png`, `reader-light-panel.png`, `reader-drawer.png`, `library.png`, `file-browser.png`, `settings-<category>[-<submenu>][-pN].png`, `reading-stats.png`, `bookorbit-catalog.png`, `sleep.png`, ...):

```sh
pio run -e picco-simulator
python3 scripts/run_simulator_smoke_test.py --env picco-simulator --no-build --picco-screens ../builds/sim-screens-baseline
```

The scenario (`src/simulator/PiccoScreensSmokeTest.cpp`) opens the reader with `--book` (default: the rendering-matrix fixture), reaches the light panel and reader drawer with the Picco swipes, and walks every Settings category and submenu page. The scenario switches the UI theme to Picco first. Captures are the logical framebuffer (black/white, no grayscale passes). It replaces the folder's previous PNGs.

## Cache Note

On first open of an EPUB, an **Indexing...** popup appears while the section cache is built in `.crosspoint/`.

If rendering looks stale after a code change, delete `./fs_/.crosspoint/` to clear simulator caches.
