# FreeInk SDK update for the Picco port

**Target agent:** `implementer` (Opus 5.5 medium). Orchestrator does all git steps and final review.

## Goal

Move the `freeink-sdk` submodule of this fork from `6993701` (2026-10-02) to `2d383f8` (2026-10-30), so
that a later change can add a Picco env. `2d383f8` is the commit CrossPoint pins for its working Picco
support (`../crosspoint-reader/freeink-sdk`). The fork's existing devices must keep compiling and keep
their behaviour.

Out of scope: any Picco env, partition table, upload script, HAL port, OTA gating, unrelated cleanup,
adding SDK libraries the fork does not need (ContentProtection, Opds, CatalogList, JsonSax,
BleKeyboardHost, HapticManager, ProgressiveJpeg), toolchain changes, and edits inside `freeink-sdk/`.

## Facts

- Working tree: `CrossInk-Bookorbit`, branch `picco`, based on `7f4f15c8`. Remotes: `origin` = user's fork,
  `upstream` = `agosez/CrossInk-Bookorbit`.
- Baseline: `pio run -e x4-pro` succeeds on `7f4f15c8`: image 5,744,928 bytes, 808,672 bytes free in the
  6,553,600-byte slot, RAM 32.1 %. The tree stayed clean.
- Baselines at `6993701` (image bytes / free): `default` 6,477,152 / 76,448; `sticky` 5,654,720 / 898,880;
  `x4-classic` 5,608,256 / 945,344. Step 0 is done.
- PlatformIO: `~/.local/bin/pio`, pioarduino core 6.1.19 as in CI; the platform penv is pinned too. Do not change
  it: core 6.2.0 fails `custom_sdkconfig` envs with `No module named 'SCons.Tool.FortranCommon'`. Platform stays at `55.03.37`; `platformio.sample.ini` in the target
  SDK still names that version (CrossPoint itself uses `55.03.311`).
- SDK diff: 161 commits, 284 files, +46150/-880; about 33k of those lines are in FreeInkFont.
- API changes are not fully catalogued; the build decides. Grep of `src/` and `lib/` found no fork use of
  `setHoldPeriodicFull` or `ensureBooted`.
- `lib_deps` uses `symlink://freeink-sdk/...`. The `InputManager` and `Imu` manifests changed only in
  description and keywords. `FREEINK_TOUCH_*` derive from `FREEINK_DEVICE_*`, so no new build flags are expected.
  `patch_wolfssl.py` and `patch_sdfat.py` patch registry deps in `.pio/libdeps`, not the SDK.
- The fork's X3/X4 images are tight on flash (`check_firmware_size.py`, 6,553,600 bytes; KOReader Sync was
  removed for space). The SDK adds UI component-style paths and an 8 KiB BMI270 config reachable from `Imu.cpp`.
- `SecureClient::reserveFullRecord()` (`FREEINK_WOLFSSL_RETAIN_INPUT_BUFFER`) is compiled only by this fork, via
  `scripts/patch_wolfssl.py:59`, and is dead code in CrossPoint. So CrossPoint's build does not prove it compiles here.
- Rules: `AGENTS.md`/`CLAUDE.md` and `.claude/CONTEXT.md` apply: HAL classes in app code, verify SDK APIs in the code,
  bump the cache version and document it in `docs/file-formats.md` when a cache layout or its contents change,
  minimal comments.

## Corrections from the audit

Step 3 was written before the audit. In this fork `2372870` affects only `sticky` (X3, X4, X4 Pro, X4 Classic set
10 MHz in `BoardConfig.h`). `6bb6ef8` and `9559ed3` apply only to GSLX680 (EEGO A4), `4677fd7` only to the Picco.

## Steps

### 0. Orchestrator: baselines at the old pin

With the submodule still at `6993701`, build `default`, `sticky` and `x4-classic` once and record image size and free
bytes per env (`x4-pro` is above). A baseline env that fails is reported as already broken, not attributed to the SDK.

### 1. Orchestrator: move the pin

```sh
git -C freeink-sdk checkout --detach 2d383f8463c0da313f237b4b0de225b05d55f2d6
```

The object is already in the fork's submodule. The lane never touches git state, including inside the submodule.

### 2. Lane: make it compile

Run `~/.local/bin/pio run -e x4-pro`, in the foreground, with a timeout of 600000 ms; pipe through
`tail`/`grep` for errors and keep the full log in the session scratchpad. Fix compile and link errors in
fork-owned files only (`src/`, `lib/`, `include/`, `platformio.ini`, `scripts/`), with the smallest change that
preserves behaviour. When the SDK replaced an API, read how CrossPoint adapted in
`../crosspoint-reader` (read-only) and follow it where it fits. Add a library to `lib_deps` only if the
linker or compiler demands it, and report why.

Stop and report rather than improvise if a fix would need an SDK edit, a toolchain change, a feature
removal, or a behaviour decision. That includes a compile failure in the `reserveFullRecord` wolfSSL path: never drop
the `FREEINK_WOLFSSL_RETAIN_INPUT_BUFFER` define, because the fork's #96 download fix relies on `patch_wolfssl.py`.
Budget: at most 3 builds.

### 3. Lane: behaviour audit

Compiling does not prove these unchanged, and no device is available for testing. Read the SDK diff
(`git -C freeink-sdk diff 699370183fa3a0e33c9cb83a36f701bbb6022095 HEAD -- <path>`) against the fork's callers
and report for each area: unchanged, adapted, or risk needing a device test.

- **Storage:** `SdmmcBlockDevice.cpp`: `543a8d6` raises the SDMMC clock from 20 to 40 MHz on every SDMMC board
  (`x4-pro`, `x4-classic`), measured upstream on one X4 Pro only. Also `gpio_hold_dis` on bus pads at `begin()` and
  `sd.powerCycleOnMount`. Check against `HalStorage` and the fork's SDMMC settings.
- **Display:** `lib/hal/HalDisplay.*` and sleep/gray paths against `FreeInkDisplay`: `2372870` drops the `Ssd1677Driver`
  default SPI clock from 40 to 20 MHz for every profile with `displaySpiHz = 0` (X4 `default`, Sticky, X4 Pro SSD1677);
  new boot-time Sticky panel probe on the SD-shared bus; gray cleanup, secondary-buffer release, gray-on-glass.
- **Input:** `HalGPIO`, `MappedInputManager`, `src/main.cpp` `wasHomeKeyLongPressed` against `InputManager`: `6bb6ef8`,
  `9559ed3`, `4677fd7` (GT911 home key, touch classification, long-press hold-over), multi-touch contract, I2C bus lock.
- **Power:** `HalPowerManager` (`armPowerButtonWakeup`, `HalPowerManager.cpp:180`), `HalGPIO::isUsbConnected` against
  `PowerManager`, `BatteryMonitor`, `FrontlightManager`: `ed88f03` keeps the RTC peripheral domain on and sets RTC pulls on
  S3 ext1 wake pins, which can change deep-sleep current on `sticky`, `x4-pro`, `x4-classic`. Also external-power
  reporting and frontlight deep sleep.
- **Network:** `BookOrbitCatalogClient`, `BookOrbitSyncClient`, `HttpDownloader`, `OtaUpdater` against `SecureNet`:
  `connect()` now calls `stop()` first and pre-checks DNS, no TLS 1.2 retry after a TCP failure, response headers capped
  at 8 KB/64 entries returning -1, TLS buffer reservations, `HAVE_MAX_FRAGMENT`. BookOrbit downloads on low-heap devices
  depend on this.
- **Board profiles:** `BoardConfig.h` changes for X3, X4, X4 Pro, X4 Classic, Sticky, and fork code that duplicates
  or works around something the SDK now does itself.
- **Fonts:** `eb9b10e`: `FtFont::loadGlyph` no longer fails for glyph 0, so `metrics26_6`/`rasterize26_6` of a missing
  codepoint return `.notdef`. CrossPoint adapted in `4c9ec287` (`lib/EpdFont/TtfEpdFont.cpp`:
  `if (gid == 0 && cp != REPLACEMENT_GLYPH) return nullptr;`, section cache version bump). Check every path in
  `lib/ScalableFont/HalScalableFont.cpp` (`hasGlyph` gates near lines 777 and 852, bare `cp` into `glyphMetrics` near line
  792). If cached advances can change, bump `lib/Epub/Epub/Section.cpp:36` and document it per the cache rule.
  Also check build flags against the FreeType config (`FREEINK_FONT_ENABLE_*`).

Each named item gets an explicit verdict. Fix only real regressions, within the fork-owned files and step 2's stop rule.
List the rest, each with a concrete device test (device, path, expected result).

### 4. Orchestrator: review and verify

Read `git diff`, then build every released env once, after review: `x4-pro`, `sticky`, `default`, `x4-classic`.

## Acceptance

- `freeink-sdk` at `2d383f8`; no dirty files inside the submodule.
- All four envs build; `check_firmware_size.py` and `check_app_touch_gate.py` run as post-actions of a green build.
- Size change per env reported against its baseline. If `default` no longer fits, that does not block this Picco-only
  branch, but it must be reported, with the overflow in bytes. No feature is removed and no `FREEINK_UI_COMPONENT_STYLE`
  override is added to make it fit; that is the user's decision.
- Audit report delivered, with a verdict for each named item. Nothing is called working on hardware; compile success is
  reported as compile success. `simulator` and `test/` are outside acceptance and reported as not run.

## Commit

Orchestrator: one commit on `picco`, pushed to `origin` only (`git push -u origin picco`). Nothing goes to `upstream`.
Contains the submodule bump, any fork adaptations, and this plan. If the lane stops on a blocker, nothing is
committed and the blocker goes to the user.

## Rollback

`git -C freeink-sdk checkout --detach 699370183fa3a0e33c9cb83a36f701bbb6022095` and discard the fork-side edits.
