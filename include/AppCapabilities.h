#pragma once

// The app deliberately has a build-time touch capability, separate from a
// runtime probe. Button-only images must not retain the touch UI merely because
// the shared source tree also builds for Sticky.
#ifndef CROSSINK_APP_CAP_TOUCH
#error "Define CROSSINK_APP_CAP_TOUCH as 0 or 1 in the PlatformIO environment"
#endif

#if CROSSINK_APP_CAP_TOUCH != 0 && CROSSINK_APP_CAP_TOUCH != 1
#error "CROSSINK_APP_CAP_TOUCH must be 0 or 1"
#endif

#ifndef CROSSINK_APP_CAP_USB_DRIVE
#error "Define CROSSINK_APP_CAP_USB_DRIVE as 0 or 1 in the PlatformIO environment"
#endif

#if CROSSINK_APP_CAP_USB_DRIVE != 0 && CROSSINK_APP_CAP_USB_DRIVE != 1
#error "CROSSINK_APP_CAP_USB_DRIVE must be 0 or 1"
#endif

// The OPDS browser, its server settings and its /api/opds web endpoints. On by default;
// this fork's X3/X4 builds turn it off to fit the C3 app partition (platformio.ini).
// Only the entry points are guarded: with them gone, the linker drops the OPDS code,
// and saved servers stay on the card for a build that has OPDS again.
#ifndef CROSSINK_APP_CAP_OPDS
#define CROSSINK_APP_CAP_OPDS 1
#endif

#if CROSSINK_APP_CAP_OPDS != 0 && CROSSINK_APP_CAP_OPDS != 1
#error "CROSSINK_APP_CAP_OPDS must be 0 or 1"
#endif

// Native simulator BoardConfig intentionally exposes only simulated runtime
// profiles, so keep this firmware-image identity available at the app layer.
#if defined(FREEINK_DEVICE_X4CLASSIC) && FREEINK_DEVICE_X4CLASSIC
#define CROSSINK_APP_DEVICE_X4CLASSIC 1
#else
#define CROSSINK_APP_DEVICE_X4CLASSIC 0
#endif

// picco-simulator sets CROSSINK_APP_DEVICE_PICCO itself: the simulator has no Picco
// board profile, and FREEINK_DEVICE_PICCO stays reserved for Picco hardware code.
#if defined(SIMULATOR) && defined(CROSSINK_APP_DEVICE_PICCO)
#elif defined(FREEINK_DEVICE_PICCO) && FREEINK_DEVICE_PICCO
#define CROSSINK_APP_DEVICE_PICCO 1
#else
#define CROSSINK_APP_DEVICE_PICCO 0
#endif

#if CROSSINK_APP_DEVICE_PICCO != 0 && CROSSINK_APP_DEVICE_PICCO != 1
#error "CROSSINK_APP_DEVICE_PICCO must be 0 or 1"
#endif

// The Picco UI (PICCO theme, reader sheet). Set by the picco and picco-simulator envs.
#ifndef CROSSINK_APP_PICCO_UI
#define CROSSINK_APP_PICCO_UI 0
#endif

#if CROSSINK_APP_PICCO_UI != 0 && CROSSINK_APP_PICCO_UI != 1
#error "CROSSINK_APP_PICCO_UI must be 0 or 1"
#endif

#if CROSSINK_APP_PICCO_UI && !CROSSINK_APP_DEVICE_PICCO
#error "CROSSINK_APP_PICCO_UI requires a Picco image"
#endif

// The image can rewrite itself only with a second OTA slot to write into. The
// Picco's stock table has one, so its OTA and SD update routes do not exist.
#define CROSSINK_APP_CAP_SELF_UPDATE (!CROSSINK_APP_DEVICE_PICCO)

// Native simulator BoardConfig deliberately has no FREEINK_CAP_TOUCH macro.
// Firmware builds must keep the app and SDK capability selections in lockstep.
#if !defined(SIMULATOR)
#include <BoardConfig.h>
#if CROSSINK_APP_CAP_TOUCH != FREEINK_CAP_TOUCH
#error "CROSSINK_APP_CAP_TOUCH must match FREEINK_CAP_TOUCH"
#endif
#if CROSSINK_APP_CAP_USB_DRIVE != FREEINK_CAP_USB_MSC
#error "CROSSINK_APP_CAP_USB_DRIVE must match FREEINK_CAP_USB_MSC"
#endif
#endif

// Every button-only reader uses the full-screen menu and its bounded sample preview.
#define CROSSINK_APP_READER_SAMPLE_PREVIEW (!CROSSINK_APP_CAP_TOUCH)
