# First Picco build of CrossInk-Bookorbit

**Target agent:** `implementer` (Opus 5.5 medium). Orchestrator does all git steps and final review.

## Goal

Add a `picco` env that builds CrossInk with BookOrbit for the BOOX Picco, can never overwrite anything but the app
slot, and boots to Home in the `eink-qemu` Picco machine. Hardware-safety behaviour goes in now, because this image will
later reach a real device. No flashing to hardware is part of this plan.

## Facts

- Tree: `CrossInk-Bookorbit`, branch `picco`, `d4b7ffef`, `freeink-sdk` at `2d383f8` (has `FREEINK_DEVICE_PICCO`,
  `BoardConfig::PICCO`, `isPicco()`). Sibling `../crosspoint-reader` is the read-only reference, with working Picco
  support: commits `261726f6`, `49b84fdc`, `471243b2`, `fb2a8a8f`, `8f4d035c`. Its source has diverged from this fork, so
  those patches are ported by hand.
- PlatformIO: `~/.local/bin/pio`, pioarduino core 6.1.19 as in CI. Do not change it. Platform stays `55.03.37`.
- Current sizes: `x4-pro` 5,759,728; `default` 6,511,904 bytes (41,696 free).
- Picco table (`../crosspoint-reader/partitions_picco.csv`): `safeboot` factory 0x20000/0xC0000, `app` ota_0
  0xE0000/0xE70000. One OTA slot only.
- `esp_ota_get_next_update_partition()` on one OTA slot answers with the running slot. `FirmwareFlasher` erases and
  writes raw, so it would erase the running code. Analysis from `github.com/ma-r-s/crossplay/pull/320`, not tested on a
  device. That repo has no detected licence: use it as a reference only and write the code here yourself.
- `../crosspoint-reader/scripts/app_only_upload.py` is broken (crosspoint-reader#4248): it only clears
  `FLASH_EXTRA_IMAGES`, while the builder already appended `<offset> <path>` pairs to `UPLOADERFLAGS`, so the upload still
  writes 0x0, 0x8000 and 0xE000. Do not copy it.
- `scripts/check_firmware_size.py` compares against the smallest `app` partition, which would be `safeboot` (768 KB).
- Repo rules in `AGENTS.md`/`CLAUDE.md` apply: capability gating over scattered macros, `tr()` for UI text, no edits to
  generated files, minimal comments.

## Out of scope

Slide-switch setting and handling, USB charge strip and `holdWhileUsbPowered`, `BwFrameKeeper`, volume-rocker mapping,
release and CI workflows, `default_envs`, simulator profile, repartitioning, edits in `freeink-sdk/`, platform bump.

## Steps

### 1. Lane: build system

- Copy `partitions_picco.csv` to the repo root.
- Add `[env:picco]` modelled on `[env:x4-classic]`/`[env:x4-pro]` (`extends = base`, no `firmware_tuned`,
  `FreeInkFont` in `lib_deps`, scalable-font flags, `dio_opi`), with:
  - `custom_firmware_device_type = picco`, `CROSSINK_FIRMWARE_DEVICE_TYPE=\"picco\"`
  - `board_build.partitions = partitions_picco.csv`, `board_upload.maximum_size = 15138816` (0xE70000)
  - `extra_scripts = ${base.extra_scripts}` plus `post:scripts/app_only_upload.py`
  - `FREEINK_DEVICE_PICCO=1`, `CROSSINK_APP_CAP_TOUCH=1`, `FREEINK_CAP_USB_MSC=1`, `CROSSINK_APP_CAP_USB_DRIVE=1`,
    `ARDUINO_USB_MODE=1`, `USE_BLOCK_DEVICE_INTERFACE=1`, `BOARD_HAS_PSRAM`, `FREEINK_FB_PSRAM=1`,
    `USB_PRODUCT=\"CrossInk_Picco\"`, `USB_MANUFACTURER=\"CrossInk\"`, `ENABLE_SERIAL_LOG`, `LOG_LEVEL=1`
  - Verify in the SDK that `FREEINK_FB_PSRAM` and the USB flags are right for this board; report any deviation.
- New `scripts/app_only_upload.py`: remove the extra images from `FLASH_EXTRA_IMAGES` **and** `UPLOADERFLAGS`. As a pre-action on
  `upload`, refuse unless no `^0x` token remains in `UPLOADERFLAGS` and `env.subst("$ESP32_APP_OFFSET")` equals the `ota_0` offset from
  the partition CSV. Compare offsets as integers (`int(x, 16)`), since one is `0xe0000` and the other `0xE0000`. Fails closed.
- The same script blocks the platform's other flash-writing targets, which bypass `UPLOADERFLAGS` (pioarduino 55.03.37
  `builder/main.py:1824-1868`). At script load time, before any build step: if `COMMAND_LINE_TARGETS` contains `erase`,
  `erase_upload`, `uploadfs` or `uploadfsota`, print exactly `app_only_upload: refusing target on single-slot board` and `env.Exit(1)`.
  Alias pre-actions are not enough: `uploadfs` fails earlier on the missing `data/` directory, which would look like a refusal. Also
  `env.Replace(ERASECMD=...)` with a failing command as a second barrier. Read the platform's `builder/main.py` under
  `~/.platformio/platforms/`; block any other target that writes flash, and report it.
- `scripts/check_firmware_size.py`: limit = smallest `ota_*` app partition; only with no `ota_*` partition, the smallest
  `app` partition. Results for the existing tables must stay the same.

### 2. Lane: block every self-flash path

A Picco image must not be able to erase its own slot.

- Add `src/network/UpdateSlot.h`: `updateSlot()` returns `esp_ota_get_next_update_partition(nullptr)`, or `nullptr` when
  that equals `esp_ota_get_running_partition()`. Use it at every update-target lookup: `FirmwareFlasher.cpp:267,328`,
  `SdFirmwareUpdateActivity.cpp:82`, `OtaUpdater.cpp:430`. Grep all of `src/` and `lib/` for `esp_ota_`, `Update.begin`,
  `esp_partition_erase`, `esp_partition_write`, and handle or report every hit, including `OtaBootSwitch.cpp`. Keep
  behaviour on two-slot boards unchanged, and make the simulator keep building (`SIMULATOR` guard if the headers require it).
- UI: hide `CheckForUpdates` and `SdFirmwareUpdate` on the Picco (`SettingsList.h:1067-1068`, `SettingsActivity.cpp`), matching
  `261726f6`. Check for any other route to them: quick actions, global actions, the web server, USB serial transfer, boot-time
  update checks, `FirmwareFlasher` callers. Remove or block each on the Picco and list it.
- Recovery: `recoveryFirmwareMode` in `src/main.cpp:1597` is false on the Picco.

### 3. Lane: device gates

Find each with `git grep` rather than trusting this list, and compare against the CrossPoint commits above.

- `FirmwareBoardTag.cpp`: `FREEINK_DEVICE_PICCO` gives `"picco"`; update `test/firmware_board_tag` if it enumerates boards.
- `CrossPointSettings.cpp:285` device name.
- `UsbSerialJtagHandoff.cpp:7`: add the Picco so USB Drive's handoff compiles for it.
- `lib/hal/HalGPIO`: `isUsbConnected` uses `BatteryMonitor::isExternalPowerPresent(&known)` on the Picco (no `usbDetect` pin);
  wake-reason logic at `HalGPIO.cpp:409`; `hasToggleSwitch`/`isToggleSwitchOn` are not needed yet.
- `src/main.cpp`: Picco power-button wake with USB connected does not go back to sleep (`261726f6` `PowerButton` case). Decide
  the `AfterUSBPower` route the same way CrossPoint does.
- Touch/button UI: the Picco has Up, Down, Power, a provisional Confirm on GPIO5, and touch. Compare `MappedInputManager`,
  `SettingsActivity` long-press-menu hiding (`471243b2`) and `ActivityManager` frontlight gating (`fb2a8a8f`), and make Home,
  Settings and the reader reachable by touch. Port only what that needs.

### 4. Lane: hardware safety

Port from `261726f6` and `8f4d035c`:

- `HalPowerManager::begin`: call `BatteryMonitor::configureCharger()` (stock SGM41562 charge limits).
- `HalPowerManager::startDeepSleep` and `enterDeepSleep`: `Frontlight.setOn(false)` and `gpio.prepareForDeepSleep()` before sleep, in
  the order CrossPoint uses (shared I2C; also sleeps the TMA525C). The fork's `HalGPIO` lacks that method: add it as a one-liner over the
  SDK's `InputManager::prepareForDeepSleep()`, as CrossPoint does. Compare the fork's three `startDeepSleep` call sites and its
  `keepClockInSleep` argument.
- `HalFrontlight::begin`: brightness before warmth (`8f4d035c`), if the fork's order differs. Must not change other boards' behaviour.
- Audit `HalStorage::shutdown()` (`HalStorage.cpp:226`, from `powerDownForDeepSleep()` in `main.cpp`) and the rail handling (`powerEnable`
  GPIO0, active-low) against CrossPoint's Picco path, and report differences.

### 5. Lane: docs

- `docs/installation.md`: Picco section. Only the Boox flasher's OTA mode or CrossPoint flash tools; never esptool at `0x10000`; the `erase`,
  `erase_upload`, `uploadfs` and `uploadfsota` targets are blocked and must not be worked around; no OTA or SD update; warranty note. State that this build has
  not run on a device.
- `README.md` supported-devices note and a `CHANGELOG.md` entry under an unreleased section.

### 6. Lane: build

`~/.local/bin/pio run -e picco`, foreground, timeout 600000 ms, at most 3 builds, logs to the session scratchpad. Stop and report on a blocker:
SDK edit, platform bump, feature removal, or a behaviour decision.

### 7. Orchestrator: review and verify

Read `git diff`, then:

- `pio run -e picco`, `-e x4-pro`, `-e default`, confirming the existing sizes change only marginally.
- Precondition for the next two: `ls /dev/cu.* | grep -i usb` prints nothing, meaning no device attached.
- Upload dry run, exactly: `pio run -e picco -t upload --upload-port /dev/nonexistent-picco -v`. The esptool line must contain only `0xe0000`
  and fail on the missing port.
- Refusal dry runs: `pio run -e picco -t erase --upload-port /dev/nonexistent-picco`, the same with `erase_upload`, `uploadfs` and `uploadfsota`.
  Each must print the exact refusal line and exit non-zero. A non-zero exit without that line is a failure, not a refusal.
- `firmware-picco.bin` starts with `0xE9` and fits in 0xE70000.
- User: load `firmware-picco.bin` in `eink-qemu` (`picco` machine); the assistant has no permission to run it.

## Acceptance

- `picco`, `x4-pro`, `default` build, each passing the size check for its own table.
- Upload dry run writes only `0xe0000`; `erase`, `erase_upload`, `uploadfs` and `uploadfsota` print the refusal line. The offset refusal check is proven by
  reading the script logic (or by a temporary run if cheap).
- `updateSlot()` is used at every update-target lookup, and no UI or other route to a self-flash remains on the Picco, with the list reported.
- User confirms in the emulator: boots to Home, Settings open by touch, **BookOrbit Sync** visible, an EPUB opens, no crash in the console.
- Nothing is called working on hardware. Reported as hardware-only risks: vendor bootloader booting an app built with platform `55.03.37`
  (CrossPoint ships `55.03.311`), SSD2677 panel variant, Confirm/GPIO5 role, frontlight, charging, GPIO0 rail, sleep current, and Arduino's
  `initArduino()` possibly erasing the stock `nvs` partition (not a brick; CrossPoint shares it). The emulator proves none of these.

## Commit

Orchestrator: one commit on `picco`, pushed to `origin` only, before the emulator check, since that needs only the build. If the lane stops on a
blocker, nothing is committed and the blocker goes to the user. Not flashed anywhere.

## Rollback

`git reset --hard d4b7ffef` on `picco` before push; after push, a revert commit.
