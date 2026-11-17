#ifdef SIMULATOR

#include "SimulatorSlideSwitch.h"

#include <SDL.h>

#include <cstdlib>
#include <cstring>

namespace {
bool initialized = false;
bool switchOn = false;
bool keyWasDown = false;

void initialize() {
  if (initialized) return;
  initialized = true;
  const char* start = std::getenv("CROSSPOINT_SIM_SLIDE_SWITCH");
  switchOn = start && (std::strcmp(start, "on") == 0 || std::strcmp(start, "1") == 0);
}
}  // namespace

bool simulatorSlideSwitchOn() {
  initialize();
  const uint8_t* keys = SDL_GetKeyboardState(nullptr);
  const bool keyDown = keys != nullptr && keys[SDL_SCANCODE_L];
  if (keyDown && !keyWasDown) switchOn = !switchOn;
  keyWasDown = keyDown;
  return switchOn;
}

void simulatorSetSlideSwitch(const bool on) {
  initialize();
  switchOn = on;
}

#endif
