#pragma once

#include <cstdint>

namespace TouchscreenEscapeHatch {

// What the Up + Down chord does.
enum class Route : uint8_t {
  MappedAction,  // Run the user's Up + Down mapping.
  EnableTouch,   // Touch is locked here: turn it back on.
};

struct Inputs {
  bool hasTouchHardware;
  bool touchDisabled;  // SETTINGS.disableReaderTouchscreen
  bool isReader;
  bool globalLock;  // Touch lock applies app-wide (Picco), not only in the reader.
  bool quickLocked;
  bool modalOwnsInput;
};

inline constexpr Route routeFor(const Inputs& in) {
  if (in.modalOwnsInput || in.quickLocked || !in.hasTouchHardware || !in.touchDisabled) return Route::MappedAction;
  return in.isReader || in.globalLock ? Route::EnableTouch : Route::MappedAction;
}

}  // namespace TouchscreenEscapeHatch
