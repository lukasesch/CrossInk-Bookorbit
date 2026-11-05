# Merge CrossInk release/v2.0.0 into the BookOrbit fork (Picco branch)

**Target agents:** lane A `implementer` (code), lane B `grunt` (docs and community translations).
Orchestrator owns all git state and the final review.

## State (2026-11-05)

- Repo `CrossInk-Bookorbit`, branch `sync/v2.0.0` (from `picco` `5347a3e3`), **merge in progress** of
  `crossink/release/v2.0.0` (`b8feaec4`, uxjulia/CrossInk). Merge base is the `v1.6.1` tag; no graft.
- `freeink-sdk` is already resolved to upstream `684ea60` (contains our old `2d383f8`). Do not touch it.
- 152 files contain conflict markers: `git diff --name-only --diff-filter=U`. Read-only git is allowed
  (`git show :1:<f>` base, `:2:<f>` ours, `:3:<f>` theirs, `git log crossink/release/v2.0.0 -- <f>`,
  `git show <sha>`). **Never** run `git add`, `checkout`, `merge`, `stash`, `reset`, `commit`; just edit the
  files until no markers remain. The orchestrator stages.
- Procedure: global skill `fork-upstream-sync` (read it: `~/.claude/skills/fork-upstream-sync/SKILL.md`) and
  `.claude/CONTEXT.md` lines 60-65 (fork traps from earlier syncs). Hunk by hunk; never take a whole file from
  one side. Upstream's Picco code arrived in squash commit `2c0d7056`; its PR intent is not linkable.
- Fork chain: uxjulia/CrossInk → agosez/CrossInk-Bookorbit → lukasesch (this repo). The fork's identity:
  BookOrbit sync/catalog/highlights/stats, KOReader Sync removed, clock on clockless boards, Cloudflare Access,
  plus this branch's Picco work.

## Resolution policy

Upstream wins (take theirs, drop ours, re-point callers):
- `HalFrontlight::initializeHardware()` replaces our `probe()` (also the simulator header and the `main.cpp`
  call before `SETTINGS.loadFromFile()`).
- Picco gestures: upstream's `usesFrontlightSwipeLayout()` scheme everywhere. Remove our
  `readerTopEdgeOpensLightPanel`, `wasReaderTopEdgeLightPanelGesture`, its `ActivityManager` wiring and its
  test case; keep any test infrastructure upstream still uses.
- `partitions_picco.csv`, `env:picco` and `env:picco-debug` from upstream (keep `CROSSINK_FIRMWARE_DEVICE_TYPE` and any
  fork-only flags the other fork envs carry, e.g. KOReader-off / BookOrbit switches; compare with `env:x4-pro`).
- OTA/SD self-flash guards: upstream's inline `dest == running` checks. Drop our `src/network/UpdateSlot.h` if every
  use is covered; keep it only where upstream has no guard, and say where.
- Picco power/USB/battery/charger code in `lib/hal/HalGPIO*`, `HalPowerManager*`, `InputWake` from upstream
  (`holdWhileUsbPowered`, charger config, wake pins, battery estimates).

Ours wins or is combined:
- `scripts/app_only_upload.py`: upstream's script as the base, plus our guards: refuse `erase`, `erase_upload`,
  `uploadfs`, `uploadfsota` at load time with the exact line `app_only_upload: refusing target on single-slot board`
  and `env.Exit(1)`, the failing `ERASECMD`, and the pre-upload check that no `^0x` image other than the app offset
  remains. Keep `debug_load_mode = manual` on both Picco envs.
- `scripts/check_firmware_size.py`: must judge Picco against the `app` (ota_0 / `custom_firmware_app_partition`)
  slot, not `safeboot`; take whichever side does that, results for other tables unchanged.
- Slide switch: **our level semantics** (position = state, `slideSwitchOwns()` hides and neutralizes competing
  controls, applied at boot/wake). Adopt upstream's enum (`enum class SlideSwitchAction { Light, NightMode,
  TouchControls, TiltPageTurn }`, same key `slideSwitchAction`, same order) and its fourth binding TiltPageTurn,
  with level semantics too (position on = tilt page turn on; hide the tilt toggle while owned). TouchControls drives
  the field upstream drives (`disableReaderTouchscreen`, inverted as upstream does). Keep upstream's switch wake pin.
  Delete `SlideSwitchToggle.h` if nothing uses it.
- `Display > Frontlight > Brightness & Warmth` entry: keep.
- All BookOrbit features, the first-open auto-sync and the `BookOrbitSmartSync::decide(..., localAtStart)` rule: keep.
- **Shortcut IDs:** upstream appended to `SHORT_PWRBTN` (now up to `ROTATE_FLIP = 39`), `POWER_CHORD_ACTION`
  (to 36) and maybe `LONG_PRESS_MENU_ACTION`; ours collide (`BOOKORBIT_SYNC = 37`, `CHORD_BOOKORBIT_SYNC = 33`,
  `LONG_MENU_BOOKORBIT_SYNC = 25`). Follow `.claude/CONTEXT.md:62` exactly: move ours past upstream's, extend
  `migrateBookOrbitShortcutValues()`, bump `BOOKORBIT_SHORTCUT_IDS_CURRENT`, align `ButtonShortcutController::ChordAction`,
  update `SimulatorSmokeTest.cpp` ID assertions. Add a native test for the migration if a test target for settings exists.
- `BASE_SETTINGS_CAPACITY`: recount (CONTEXT:63). Clock/date gates: CONTEXT:64.
- **Upstream's new CrossPoint Sync provider:** merge it as upstream has it, but if it competes with BookOrbit in a
  menu/shortcut slot the fork repurposed, or if `default` does not fit, stop and report (see Build).
- `lib/I18n/translations/english.yaml`: union of keys (upstream's text for shared keys, fork keys kept).
  Upstream now treats only `english.yaml` + `languages.json` as build inputs (`AGENTS.md` Generated Files); follow
  `scripts/gen_i18n.py` from upstream.
- CI/release (`.github/workflows/*`, `scripts/generate_release_catalog.py`): upstream's new jobs adopted, the fork's
  release naming/targets (BookOrbit release, agosez/lukasesch repo URLs) kept.
- `AGENTS.md`/`CLAUDE.md`: this fork's versions stay (CLAUDE.md is `@AGENTS.md`); add upstream's new rules to AGENTS.md
  only where they are general (e.g. the i18n line).

## Lane split (file ownership)

- **Lane B (grunt):** `CHANGELOG.md` (fork's file wins entirely; add under `[Unreleased]` "Based on CrossInk v2.0.0
  (pre-release `release/v2.0.0` `b8feaec4`)"), `README.md` (fork's README; take upstream facts it repeats, e.g.
  device list now including the Picco officially), `docs/**`, `.gitignore`, every `lib/I18n/translations/*.yaml`
  **except** `english.yaml` (union of keys; upstream wording for shared keys; fork's own keys kept). No builds.
- **Lane A (implementer):** everything else.

## Build and test (lane A)

- Native: `cmake -S test -B build/test && cmake --build build/test -j8 && ctest --test-dir build/test` (configure is
  done; re-run configure if `test/CMakeLists.txt` changed). All must pass; report any that fail with the reason.
- Firmware, one command, foreground, timeout 600000 ms, log outside the repo:
  `~/.local/bin/pio run -e picco -e x4-pro -e default -e sticky -e x4-classic`. At most 4 runs. Then `simulator` once
  (CONTEXT:13: missing stub symbols are a known local issue; report, do not patch libdeps).
- Report per env the `check_firmware_size` line ("fits ... bytes free"), **not** PlatformIO's Flash line.
  If `default` does not fit or has < 8 KB free: stop, report the sizes and the largest new upstream contributors
  (`pio run -e default -t size` or map file), do not remove features.
- Format only changed lines: `git clang-format --diff --extensions h,cpp | git apply` is a working-tree patch; allowed.
  (It needs a staged index to compare against? If it refuses because of the merge, skip and report.)

## Report

Per lane: conflict files resolved, every non-trivial resolution with the upstream commit it follows or the reason it
rejects it, files still holding markers, build/test results. Lane A additionally: the shortcut-ID migration, the
switch semantics as merged, size table. No diff.
