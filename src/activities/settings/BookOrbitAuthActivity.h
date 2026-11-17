#pragma once

#include <functional>
#include <string>

#include "AppCapabilities.h"
#include "activities/Activity.h"
#include "components/themes/BaseTheme.h"

/**
 * Activity for testing BookOrbit credentials.
 * Connects to WiFi and authenticates with the BookOrbit server.
 */
class BookOrbitAuthActivity final : public Activity {
 public:
  explicit BookOrbitAuthActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("BookOrbitAuth", renderer, mappedInput) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
  bool preventAutoSleep() override { return state == CONNECTING || state == AUTHENTICATING; }

 private:
  // FORM: the Picco sign-in form (T_Login); the other themes start in WIFI_SELECTION.
  enum State { FORM, WIFI_SELECTION, CONNECTING, AUTHENTICATING, SUCCESS, FAILED };

  State state = WIFI_SELECTION;
  std::string statusMessage;
  std::string errorMessage;

  void onWifiSelectionComplete(bool success);
  void performAuthentication();

#if CROSSINK_APP_PICCO_UI
  static constexpr int kPiccoFields = 3;  // server, user name, password
  static constexpr int kPiccoStops = kPiccoFields + 1;
  bool picco = false;
  int piccoFocus = -1;  // rocker focus: a field or the Sign in button; -1 = none
  Rect piccoFieldRects[kPiccoFields];
  Rect piccoButtonRect;
  void startSignIn();
  void openField(int field);
  void renderPicco();
  void loopPicco();
#endif
};
