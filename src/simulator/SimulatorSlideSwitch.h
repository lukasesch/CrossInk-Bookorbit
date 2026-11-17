#pragma once

#ifdef SIMULATOR

// picco-simulator stand-in for the Picco slide switch: L toggles it, and
// CROSSPOINT_SIM_SLIDE_SWITCH=on starts it on. Polled each main-loop pass through
// deviceSlideSwitchOn(), like the real switch level.
bool simulatorSlideSwitchOn();
void simulatorSetSlideSwitch(bool on);

#endif
