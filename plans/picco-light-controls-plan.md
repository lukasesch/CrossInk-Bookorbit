# Picco light controls: brightness/warmth entry and slide switch

**Target agent:** `implementer` (Opus 5.5 medium). Orchestrator does all git steps and the final review.

## Goal

On the BOOX Picco (env `picco`), the user can reach brightness and warmth from Settings and turn the light on and off with
the hardware slide switch, as CrossPoint does. Other boards keep their behaviour, except that every board with a
frontlight gains the new Settings entry.

## Facts (verified 2026-11-05)

- Tree: `CrossInk-Bookorbit`, branch `picco`, HEAD `b4fec27f`. `freeink-sdk` must not be edited. Sibling
  `../crosspoint-reader` (MIT) is the read-only reference; port by hand, its source has diverged.
- Real device status: boots, display/touch/rocker/sleep/wake work. `b4fec27f` fixed `HalFrontlight::begin` so
  `Frontlight.present()` is true on the Picco; `Display > Frontlight` now appears, with only Restore Light on Wake,
  Schedule, Start, End. Multi-touch works: two-finger swipe brightness/warmth works in the reader.
- Brightness/warmth are edited only in `FrontlightPanelActivity` (`src/activities/util/`). It opens from
  `openFrontlightPanel()` (file-static, `src/activities/ActivityManager.cpp:155`) via the top-edge gesture, which
  `MappedInputManager::wasLightPanelGesture()` (`src/MappedInputManager.cpp:843`) allows only on home-key boards; without a
  home key the top edge is the menu gesture (`:774`). The Picco has no home key, so the panel is unreachable. Do not
  change the gestures.
- `Display > Frontlight` submenu: `buildGroupedDisplaySettingsList` (`src/SettingsList.h:997`, shown when
  `Frontlight.present()`), contents from `buildDisplayFrontlightSettingsList` (`src/SettingsList.h:1015`), handled in
  `SettingsActivity.cpp` (`SettingAction::DisplayFrontlight`, lines ~405, ~490, ~1293).
- Slide switch in the SDK: Picco profile sets it (`freeink-sdk/libs/hardware/BoardConfig/include/BoardConfig.h:1789,1815`,
  GPIO6, on = LOW, debounced); `InputManager::hasToggleSwitch()`, `isToggleSwitchOn()`, `wasToggleSwitchChanged()`
  (`InputManager.h:210-217`). The fork's `HalGPIO` exposes none of them.
- CrossPoint reference (read these, port the slide-switch parts only): `261726f6` and `8f4d035c`.
  - `src/util/SlideSwitch.h`: `slideSwitchOwns(action)`, `slideSwitchOwnsLight()`.
  - `src/CrossPointSettings.h:409-414`: `SLIDE_SWITCH_ACTION` {FRONTLIGHT, NIGHT_MODE, TOUCH_CONTROLS}, default FRONTLIGHT.
  - `src/SettingsList.cpp:143`: `STR_SLIDE_SWITCH` enum setting in Controls; `src/SettingsList.h:308-312`: hides settings the
    switch owns.
  - `src/main.cpp:223` `handleSlideSwitch()`, called at `:746` and `:889`; `:699` boot light state follows the switch when
    it owns the light.
  - `lib/hal/HalGPIO.{h,cpp}:94-95/173-175`; `FrontlightPanelActivity.cpp:116,205,276,381,429`;
    `EpubReaderMenuActivity.cpp:80,83`.
- On the device under CrossPoint the switch turns the light on and off; the user confirmed it.

## Steps

### 1. Brightness & Warmth entry

- Add an entry at the top of `Display > Frontlight` that opens the existing `FrontlightPanelActivity`, on every board where
  `Frontlight.present()`. Expose the panel opening through a proper `ActivityManager` API (or the existing result flow)
  instead of duplicating `openFrontlightPanel()`; returning from the panel goes back to the submenu.
- Label via `tr()`. Reuse an existing string if one fits (`STR_FRONTLIGHT`, brightness/warmth strings); otherwise add
  `STR_BRIGHTNESS_WARMTH` ("Brightness & Warmth", or "Brightness" when `!Frontlight.hasColorTemperature()` if a second
  string is cheap) to `lib/I18n/translations/english.yaml` and regenerate with `scripts/gen_i18n.py`. Follow how the
  repo handles keys missing from other languages.

### 2. Slide switch

- `HalGPIO`: `hasToggleSwitch()`, `isToggleSwitchOn()` over the SDK `InputManager`.
- `SETTINGS.slideSwitchAction` with CrossPoint's enum and default FRONTLIGHT, persisted the way the fork persists settings
  (check its versioning/serialization and web API exposure). Map NIGHT_MODE and TOUCH_CONTROLS to the fork's
  equivalents; drop an option that has no clean equivalent and report it.
- `Settings > Controls > Slide Switch`, shown only when `gpio.hasToggleSwitch()`.
- `handleSlideSwitch()` in the main loop and on wake, applying the level (not just edges) when it changes or the action
  changes. When it owns the light: switch on = light on at the stored brightness/warmth; off = light off;
  `SETTINGS.frontlightOn` follows; boot/wake light state follows the switch, not Restore Light on Wake or the schedule.
- While the switch owns the light, nothing else may turn the light on or off behind it. Hide or neutralize: Restore Light on
  Wake, Schedule/Start/End (and skip applying the schedule), the panel's on/off toggle, any reader-menu light toggle,
  the short-power / chord / home-key / quick-action TOGGLE_FRONTLIGHT paths, and brightness/warmth changes (panel, two-finger
  swipe `applyConfiguredSwipeAction`) must not switch the light on when the switch is off. `rg` for `setOn(` and
  `frontlightOn` to find every path; list each one and what you did.
- Same rule for NIGHT_MODE / TOUCH_CONTROLS if ported: the UI toggle the switch owns is hidden.
- Use `include/AppCapabilities.h` / `DeviceCapabilities.h` style gating; no scattered `FREEINK_DEVICE_PICCO` checks in
  activities. Simulator builds must keep compiling (`hasToggleSwitch()` false there).

### 3. Docs

- `CHANGELOG.md` `[Unreleased]`: under Added, the entry and the slide switch; update the existing Picco entry: it now runs
  on a retail unit (display, touch, buttons, frontlight, sleep/wake confirmed).
- `docs/installation.md` BOOX Picco section: replace "This build has not run on a device" with the confirmed list; keep the
  flashing rules unchanged.
- `docs/controls.md`: short Slide Switch subsection.

### 4. Build

One command, foreground, timeout 600000 ms, log to a file outside the repo:
`~/.local/bin/pio run -e picco -e x4-pro -e default`. At most 3 runs of it. Run `clang-format -i` on touched C++ files.
If `test/` has a native test for settings lists or frontlight logic, extend it; do not build a new harness.

## Out of scope

Gesture changes, volume-rocker remapping, USB charge-strip handling, SDK edits, release/CI, any flash or upload command.

## Stop and report instead of guessing

A needed SDK change, a settings-format break without a migration path, or a behaviour decision not covered above.

## Report

Findings, the list of light on/off paths and how each is handled, changed files, build result with sizes per env. No
diff, no git commands.
