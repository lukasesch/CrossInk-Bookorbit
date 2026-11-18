# How this fork differs from CrossInk

This repository is CrossInk ([uxjulia/CrossInk](https://github.com/uxjulia/CrossInk)) plus two
layers: **BookOrbit** ([agosez/CrossInk-Bookorbit](https://github.com/agosez/CrossInk-Bookorbit))
and, on top of it, the **BOOX Picco** port with its own interface. This file lists every
divergence from upstream CrossInk, where it lives and why, so an upstream sync knows what to
keep. User-facing changes are in `CHANGELOG.md`; durable gotchas in `.claude/CONTEXT.md`;
the per-phase design in `plans/`.

Current base: CrossInk v2.0.0, through agosez `v2.0.0+bookorbit.1-rc2` (merge `72eb81eb`).

## Gating at a glance

| Switch | Defined in | Means |
| --- | --- | --- |
| `CROSSINK_APP_DEVICE_PICCO` | `include/AppCapabilities.h` (from `FREEINK_DEVICE_PICCO`, or the simulator's own define) | Picco hardware behaviour: slide switch, one app slot, no Home key. App code gates on this, never on `FREEINK_DEVICE_PICCO` (off in the simulator). |
| `CROSSINK_APP_PICCO_UI` | `platformio.ini` (`picco`, `picco-debug`, `picco-simulator`) | The Picco interface is compiled in. Requires `CROSSINK_APP_DEVICE_PICCO`. |
| `UITheme::isPicco()` | `src/components/UITheme.h` | Runtime: `uiTheme == PICCO` (raw 64) in a Picco UI build. Every Picco screen branches on it, so the other themes keep upstream's screens. |
| `CROSSINK_APP_CAP_SELF_UPDATE` | `include/AppCapabilities.h` | Off on the Picco: no OTA or SD update. |
| `CROSSINK_APP_CAP_OPDS` | `platformio.ini`, `include/AppCapabilities.h` | Off on the X3/X4 only. |
| `CROSSINK_APP_CAP_KOREADER_SYNC` | `platformio.ini` (device envs only) | Off on every build: KOReader sync is out. `#if` only, never in an expression. |

## BookOrbit (agosez layer)

| Area | Files | Why |
| --- | --- | --- |
| Reading-progress, highlight, bookmark and stats sync | `lib/BookOrbitSync/`, `src/activities/reader/BookOrbitSyncActivity.*`, `src/network/ReadingSyncUpload.*`, `src/util/BookOrbitSmartSync.h` (credentials in `lib/BookOrbitSync/BookOrbitCredentialStore.*`) | The fork's purpose: sync with a BookOrbit server. Smart sync, Sync on Sleep (behind the sleep screen, `main.cpp`) and the first-open sync of catalog downloads. |
| Catalog browser and downloads | `src/activities/browser/BookOrbitCatalogBrowserActivity*`, `src/util/BookOrbitDevicePath.*`, `src/network/` download staging | Browse and download from BookOrbit. On the X3/X4 browsing and downloads run from their own boot (`NetworkBootTarget::BOOKORBIT_*`) for a clean heap (#96). |
| BookOrbit Home theme | `src/components/themes/bookorbit/`, `UI_THEME::BOOKORBIT = 8` | Dashboard-style Home. |
| BookOrbit Sync shortcut | `BOOKORBIT_SYNC`, `CHORD_BOOKORBIT_SYNC`, `LONG_MENU_BOOKORBIT_SYNC` in `src/CrossPointSettings.h`; `carryBookOrbitShortcut()` in `CrossPointSettings.cpp` | Kept after upstream's last action; moved and carried over each time upstream appends (`.claude/CONTEXT.md`). |
| Content-keyed book state | `src/util/BookContentId.*` | Highlights, bookmarks and progress survive renames and moves. |
| Clock and date without a clock chip | `lib/hal/HalClock.*`, `lib/WallClock/`, `src/activities/reader/ReadingDailyMinutes.*` | Status bar clock/date and reading-day stats on the X4. Status bar sites ask `canDisplayTime()`, not `isAvailable()`. |
| OTA from this fork, release candidates | `platformio.ini` (release URL), `src/network/OtaUpdater.*`, `src/network/FirmwareVersion.*`, `.github/workflows/release*.yml` | Updates come from the fork's releases; an opt-in RC channel. |
| Fork language and hyphenation packs | `src/activities/settings/AssetDownloadActivity.*`, `src/network/AssetCatalogSignature.*`, `src/network/AssetDownloadService.*`, `scripts/export_pages_language_assets.py` | Upstream's packs lack the fork's keys: the fork publishes and signs its own (GitHub Pages). |
| KOReader sync removed | `platformio.ini`, `src/SettingsList.h`, `src/QuickActions.h`, readers, `src/main.cpp` | Flash room on the C3 (`7b9ab497`). Upstream's Sync Server / Sync Book stay out too. |
| OPDS removed on X3/X4 | `CROSSINK_APP_CAP_OPDS` in `platformio.ini`; `SettingsList.h`, `SettingsActivity.cpp`, `ActivityManager.cpp`, network activities, `CrossPointWebServer.cpp` | v2.0.0 no longer fits on the C3 next to BookOrbit; the catalog covers the need. Saved servers stay on the SD card. |
| C3 flash diet | emoji/CJK fallback glyphs dropped (`platformio.ini`), out-of-line `PersistableStore::getInstance()` (`lib/Serialization/PersistableStore.h`), zopfli web pages | The C3 image has single-digit KB left. |
| Memory and TLS hardening | `HttpDownloader`, `SecureClient` max-fragment request, streamed annotation replies | #96 and large-highlight syncs on the C3. |
| Integration suite | `test/integration/`, `.github/workflows` integration job | Real BookOrbit server scenarios. |

## BOOX Picco hardware

| Area | Files | Why |
| --- | --- | --- |
| Build and flash safety | `[env:picco]`, `[env:picco-debug]` in `platformio.ini`, `partitions_picco.csv`, `scripts/app_only_upload.py`, `scripts/check_firmware_size.py` | One app slot (`ota_0` at 0xE0000) beside the vendor recovery; installs write only that slot, and `erase`/`uploadfs` targets refuse to run. |
| No self-update | `CROSSINK_APP_CAP_SELF_UPDATE`, `src/SettingsList.h`, `SdFirmwareUpdateActivity.cpp`, `OtaUpdater.cpp`, `FirmwareFlasher.cpp` | Same safety: nothing may write outside the app slot. |
| HAL fixes | `lib/hal/HalGPIO.*`, `lib/hal/HalFrontlight.*`, `lib/hal/HalPowerManager.cpp`, `lib/DeviceInfo/HalDeviceInfo.cpp`, `src/platform/UsbSerialJtagHandoff.cpp`, `src/main.cpp` (USB wake token) | Picco buttons, frontlight probe before settings load and request order, power latch, touch controller name, sleep after USB power-on. |
| SDK pin | `freeink-sdk` (`96ce5a7`) | Picco display waveform, gray timing and wake order. |
| Slide switch | `src/util/SlideSwitch.h`, `slideSwitchAction` setting, `src/main.cpp`, `src/simulator/SimulatorSlideSwitch.*` | A position, not a toggle: it owns the bound feature. |
| Touch escape hatch | `src/util/TouchscreenEscapeHatch.h`, `ActivityManager.cpp` | Up + Down turns touch back on everywhere, so a book can never lock touch out. |
| Simulator profile | `[env:picco-simulator]`, `test/integration/ci/patch_simulator_package.py`, `src/simulator/PiccoScreens*.cpp` | X4 Pro profile as a Picco; one PNG per screen for the visual gate (`docs/simulator.md`). |

## Picco interface

All of it is compiled only with `CROSSINK_APP_PICCO_UI` and drawn only while `UITheme::isPicco()`.
Picco screens live beside upstream's in `*Picco.cpp` files or in `src/components/picco/`, so an
upstream change to a screen rarely touches them.

| Area | Files | Gating / notes |
| --- | --- | --- |
| Theme, tokens, shared controls | `src/components/picco/` (`PiccoTheme`, `PiccoTokens`, `PiccoLayout`, `PiccoSegmented`, `PiccoListUi`, `PiccoListView`, `PiccoDialog`, `PiccoToast`, ...), `src/components/icons/picco*`, Literata/figures fonts in `lib/EpdFont/builtinFonts/` | `UI_THEME::PICCO = 64`, far outside upstream's range (`isKnownUiTheme()`). One segmented control: geometry in `PiccoSegmented.h`, drawn by `PiccoListUi` and `drawUiSegmentedTabBar()`. |
| Default theme | `CrossPointSettings::DEFAULT_UI_THEME` | A fresh settings file on a Picco UI build starts on Picco; a stored theme always wins. |
| Home | `src/components/picco/home/`, `HomeActivity.cpp` | Current book, stats card, four tiles. |
| Reader sheet | `src/activities/util/FrontlightPanelSheet.cpp`, `src/components/picco/sheet/`, `ReaderSettingsController.*`, readers (`ReaderMenuKind.h`), `ActivityManager.cpp`, `docs/picco.md` | Replaces light panel, reader menu and Quick Actions; gesture table in `docs/picco.md`. |
| Library, Files, BookOrbit catalog | `LibraryActivityPicco.cpp`, `FileBrowserActivityPicco.cpp`, `BookOrbitCatalogBrowserActivityPicco.cpp`, `SavedItemsHomeActivity.cpp` | Tabs, grid, sort page, path bar. |
| Settings and system pages | `SettingsActivityPicco.cpp`, `WifiSelectionActivityPicco.cpp`, `FontDownloadActivityPicco.cpp`, `HyphenationManagerActivityPicco.cpp`, `StatusBarSettingsActivityPicco.cpp`, `KeyboardEntryActivityPicco.cpp`, `BookOrbitAuthActivityPicco.cpp`, `PiccoStatsPage.*`, `PiccoTransferUi.*` | Sections instead of tabs, steppers, full-page pickers. |
| Plugins hidden | `CrossPointSettings.cpp` (`pluginsEnabled` forced off), `HomeActivity.cpp`, `SettingsActivityPicco.cpp` | No entry point in this interface, so plugins must not run unseen. |
| Quick Actions hidden | `QUICK_ACTIONS_BINDINGS` in `CrossPointSettings.h`, `fromJson()`, `getSettingsList()` / `buildControlsSettingsParentList()` in `SettingsList.h` | The sheet replaces it. Enum numbers stay reserved; on load a stored "Open Quick Actions" binding becomes Reader Menu (Home key) or none. The code stays: upstream keeps changing it. |
| Strings | `STR_PICCO_*` at the end of all 29 YAMLs; `lib/LanguageCache/LanguageCache.cpp` | The language-pack size check skips English-valued `STR_PICCO_` keys (other languages fall back to English for them). |

## Tooling

| Area | Files | Why |
| --- | --- | --- |
| Icon generator `--extra-svgdir` | `scripts/generate_icons.py` | Picco manifests mix Lucide icons with board-traced ones (`piccoHomeSvg/`, `piccoSystemSvg/`, `assets/picco-sheet-icons/`). |
| Agent guide | `AGENTS.md` (canonical), `CLAUDE.md` imports it | Seafile does not sync symlinks. |
