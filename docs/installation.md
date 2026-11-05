---
title: Installation
nav_order: 2
---

# Installation

## Supported Devices

- Xteink X3 and X4
- Xteink X4 Classic and X4 Pro
- Seeed Studio Sticky
- BOOX Picco: supported by CrossInk v2.0.0 (see [BOOX Picco](./picco.md)); this fork publishes no release file for it, see [BOOX Picco](#boox-picco) below.

## Firmware Files

Every path below starts from a `firmware-*.bin` downloaded from **this fork's**
[releases page](https://github.com/agosez/CrossInk-Bookorbit/releases) — one file per
device model. CrossInk's own web installer and update feeds only carry upstream builds,
which have no BookOrbit support. Once this fork is installed,
`Settings > System > Check for Updates` follows this fork's releases, so later versions
arrive over the air.

## Languages and hyphenation

You can add menu languages and hyphenation packs without erasing your reader.
Some devices keep installed packs on the SD card, so keep that card in the
reader. See [Languages](./languages.md) for setup and recovery.

## Web Installation via USB

#### For new installs and updates.

1. Download the `firmware-*.bin` for your device from the releases page.
2. Open the [CrossPoint flash tools](https://crosspointreader.com/#flash-tools), select
   your device model and choose the custom-firmware option.
3. Give it the downloaded `firmware-*.bin` and start the flash, keeping the reader
   connected through the download-mode and flashing steps.

## USB Drive

On the X4 Classic and X4 Pro, you can plug the reader into your computer and use its SD card like a USB stick. This is handy for copying books and fonts.

1. On the reader, go to **Home > File Transfer > USB Drive**.
2. Connect the reader to your computer with a USB-C cable.
3. When you are done, eject the drive on your computer first, then unplug the cable.

The reader returns to the Home screen once the drive is ejected or the cable is removed.

## SD Card Firmware Update

#### For a device that already runs CrossInk (upstream or this fork). Works on USB-locked devices.

1. Download the `firmware-*.bin` for your device from the releases page.
2. Place it anywhere on the SD card.
3. Go to `Settings > System > SD Card Firmware Update`, navigate to the `.bin` file and
   update.

## Reverting to upstream CrossInk

Flash an upstream build with CrossInk's own
[web installer](https://inky.crossink.dev/#flash-tools), or place an upstream
`firmware-*.bin` on the SD card and use the SD Card Firmware Update above. Settings,
books and reading progress live on the SD card and survive the swap in both directions.
Highlights and bookmarks stay stored too, but upstream does not read the content-keyed
files this fork keeps them in, so they are invisible there — everything returns when the
fork is flashed back.

## BOOX Picco

This build runs on a retail unit: display, touch, buttons, frontlight, and sleep and wake work.
It is still experimental and comes with no promise that it works. Flashing it may void your warranty, and you do so at your own risk.

Build it with `pio run -e picco`, which writes `.pio/build/picco/firmware-picco.bin`.

The Picco keeps its vendor bootloader, partition table and `safeboot` recovery partition. The
firmware goes only into the stock `app` slot at `0xE0000`. Install it with one of these:

- the Boox flasher's OTA mode
- the [CrossPoint flash tools](https://crosspointreader.com/#flash-tools)

Rules for this device:

- Never use the esptool command below, or any write at `0x10000`. On the Picco that address is
  inside the `safeboot` partition.
- The `erase`, `erase_upload`, `uploadfs` and `uploadfsota` PlatformIO targets are blocked for
  this build because they would destroy the stock layout. Do not work around that.
- There is no OTA or SD card update. The stock table has one app slot, so the firmware could only
  overwrite itself. `Check for Updates` and `SD Card Firmware Update` are hidden.

## Command Line

These instructions are for macOS and Linux, and for the ESP32-C3 devices (X3 and X4);
for the other devices, use the web installer.

Install `esptool`:

```sh
pip3 install esptool
```

Download the `firmware-*.bin` file from the
[releases page](https://github.com/agosez/CrossInk-Bookorbit/releases), then connect
your device with USB-C.

Find the device port:

```sh
# Linux
dmesg | grep tty

# macOS
ls /dev/cu.*
```

Flash the firmware:

```sh
# Linux
esptool.py --chip esp32c3 --port /dev/ttyACM0 --baud 921600 write_flash 0x10000 /path/to/firmware.bin

# macOS
esptool.py --chip esp32c3 --port /dev/cu.usbmodem2101 --baud 921600 write_flash 0x10000 /path/to/firmware.bin
```

Replace the port and firmware path with your actual values.
