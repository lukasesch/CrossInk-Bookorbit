#pragma once
#ifdef SIMULATOR
// Screenshot scenario: one capture per reachable screen into
// CROSSINK_SIMULATOR_SMOKE_PICCO_SCREENS (scripts/run_simulator_smoke_test.py --picco-screens).
bool tickPiccoScreensSmokeTest();
#endif
