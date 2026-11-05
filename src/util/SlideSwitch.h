#pragma once

#include <HalGPIO.h>

#include "CrossPointSettings.h"
#include "DeviceCapabilities.h"

// A slide switch owns the state of the function it is bound to, so the UI must
// not offer its own toggle for that function or change it behind the switch.
inline bool slideSwitchOwns(const CrossPointSettings::SlideSwitchAction action) {
  return deviceHasSlideSwitch(gpio) && SETTINGS.slideSwitchAction == static_cast<uint8_t>(action);
}

inline bool slideSwitchOwnsLight() { return slideSwitchOwns(CrossPointSettings::SlideSwitchAction::Light); }
