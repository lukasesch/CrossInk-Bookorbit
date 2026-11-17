#pragma once

#include "AppCapabilities.h"

#if CROSSINK_APP_PICCO_UI

#include <cstdint>
#include <functional>

#include "PiccoPagedList.h"
#include "PiccoSettingsRows.h"

class GfxRenderer;
class MappedInputManager;
class ButtonNavigator;

namespace freeink {
struct Icon;
}

// A full Picco list screen: header (back, title, optional contextual action), an optional
// note, sectioned rows paged by swipe with `n / m`, rocker focus that flips at the page edge.
// Settings, the option picker, Wi-Fi, fonts, hyphenation and the status bar share it.
class PiccoListView {
 public:
  static constexpr int kMaxPages = 16;
  // Touch ids: rows use their index; a row's framed button and the header action sit above.
  static constexpr int kStepperIdBase = 1000;  // + 2 * row (+1 for plus)
  static constexpr int kButtonIdBase = 2000;
  static constexpr int kActionId = 3000;

  struct Frame {
    const char* title = "";
    const char* note = nullptr;
    const freeink::Icon* action = nullptr;  // header contextual action (T_Wifi rescan)
    bool versionFooter = false;
    int topReserve = 0;                     // px kept free under the header for the caller (a preview card)
    const char* continuedFormat = nullptr;  // nullptr: tr(STR_PICCO_CONTINUED_FORMAT)
  };

  enum class Event : uint8_t { None, Activate, Button, Step, Action, Back, Dismiss };
  struct Input {
    Event event = Event::None;
    int index = -1;     // Activate / Button / Step: the row
    int direction = 0;  // Step: -1 minus, +1 plus
  };

  bool focus = false;           // rocker focus frame shown
  bool followSelection = true;  // next draw shows the selection's page
  int page = 0;                 // written by draw, read by input
  int pageCount = 1;
  int16_t pageStarts[kMaxPages] = {};

  void reset() {
    focus = false;
    followSelection = true;
    page = 0;
  }

  // Draws into the frame buffer and registers touch targets; the caller displays the buffer.
  void draw(GfxRenderer& renderer, const MappedInputManager& input, const Frame& frame,
            const PiccoSettingsRows::Row* rows, int count, int selected);

  // Routes back, taps, swipes and the rocker. `selected` follows taps and rocker steps;
  // `redraw` is called when the view changed. allowDismiss: a bottom-edge swipe up returns
  // Dismiss (Settings opened as a transient menu).
  Input input(MappedInputManager& input, const GfxRenderer& renderer, ButtonNavigator& navigator, const char* title,
              int count, int& selected, const std::function<bool(int)>& isRow, const std::function<void()>& redraw,
              bool allowDismiss = false);
};

#endif
