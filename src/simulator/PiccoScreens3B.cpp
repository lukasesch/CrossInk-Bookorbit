#ifdef SIMULATOR
#include "PiccoScreens3B.h"

#include "AppCapabilities.h"

#if CROSSINK_APP_CAP_TOUCH && CROSSINK_APP_PICCO_UI
#include <HalStorage.h>
#include <Logging.h>

#include <cstdlib>

#include "activities/ActivityManager.h"
#include "activities/RenderLock.h"
#include "activities/util/FrontlightPanelActivity.h"

extern ActivityManager activityManager;
extern GfxRenderer renderer;
extern MappedInputManager mappedInputManager;

namespace {
constexpr const char* kTxtPath = "/picco-sheet-smoke.txt";

FrontlightPanelActivity& sheet() {
  auto* panel = dynamic_cast<FrontlightPanelActivity*>(activityManager.simulatorCurrentActivity());
  if (!panel || !panel->simulatorIsSheet()) {
    LOG_ERR("SMOKE", "Picco screens: the reader sheet is not open");
    std::_Exit(2);
  }
  return *panel;
}

void addSwipe(const PiccoScreenHooks& h, const int x, const int fromY, const int toY) {
  h.steps->push_back([x, fromY] {
    mappedInputManager.simulatorInjectTouchDown(x, fromY);
    return 0;
  });
  h.steps->push_back([x, toY] {
    mappedInputManager.simulatorInjectTouchMove(x, toY);
    return 0;
  });
  const int settle = h.settleTicks;
  h.steps->push_back([x, toY, settle] {
    mappedInputManager.simulatorInjectTouchRelease(x, toY);
    return settle;
  });
}

void addTap(const PiccoScreenHooks& h, const int x, const int y) {
  h.steps->push_back([x, y] {
    mappedInputManager.simulatorInjectTouchDown(x, y);
    return 0;
  });
  const int settle = h.settleTicks;
  h.steps->push_back([x, y, settle] {
    mappedInputManager.simulatorInjectTouchRelease(x, y);
    return settle;
  });
}

void addPane(const PiccoScreenHooks& h, const PiccoSheet::Pane pane, const uint8_t tab, const std::string& name) {
  h.steps->push_back([h, pane, tab, name] {
    {
      RenderLock lock;
      sheet().simulatorShowSheetPane(pane, tab);
    }
    h.renderAndCapture(name);
    return 0;
  });
}

bool writeTxtBook() {
  HalFile file;
  if (!Storage.openFileForWrite("SMOKE", kTxtPath, file)) return false;
  static constexpr char kText[] =
      "She had walked the length of the garden twice before she noticed that the light had changed.\n\n"
      "There was a letter in her pocket that she had not answered.\n";
  const bool ok = file.write(reinterpret_cast<const uint8_t*>(kText), sizeof(kText) - 1) == sizeof(kText) - 1;
  file.close();
  return ok;
}
}  // namespace

void addPiccoSheetScreens(const PiccoScreenHooks& h, const std::string& bookPath) {
  const int width = renderer.getScreenWidth();
  const int height = renderer.getScreenHeight();
  using PiccoSheet::Pane;

  // EPUB: the main sheet and every pane (both tabs where there are two).
  h.steps->push_back([bookPath, h] {
    activityManager.goToReader(bookPath, true);
    return h.bookOpenTicks;
  });
  h.steps->push_back([h] {
    h.expect("EpubReader", "reopen book");
    return 0;
  });
  addSwipe(h, width / 2, height / 3, height * 2 / 3);
  h.steps->push_back([h] {
    h.expect("FrontlightPanel", "reader swipe down opens the sheet");
    h.renderAndCapture("sheet-main");
    return 0;
  });
  addPane(h, Pane::Text, 0, "sheet-text");
  addPane(h, Pane::Text, 1, "sheet-text-2");
  addPane(h, Pane::TextMore, 0, "sheet-text-more");
  addPane(h, Pane::More, 0, "sheet-more");
  addPane(h, Pane::More, 1, "sheet-more-2");
  addPane(h, Pane::GoTo, 0, "sheet-goto");
  addPane(h, Pane::AutoPageTurn, 0, "sheet-auto-page-turn");
  addPane(h, Pane::Advanced, 0, "sheet-advanced");
  addPane(h, Pane::Advanced, 1, "sheet-advanced-2");
  h.steps->push_back([] {
    RenderLock lock;
    sheet().simulatorShowSheetPane(Pane::Main, 0);
    return 0;
  });
  // A tap under the sheet closes it; a swipe up in the reader goes Home.
  addTap(h, width / 2, height - 20);
  h.steps->push_back([h] {
    h.expect("EpubReader", "tap under the sheet");
    return 0;
  });
  addSwipe(h, width / 2, height * 2 / 3, height / 3);
  h.steps->push_back([h] {
    h.expect("Home", "reader swipe up goes Home");
    return 0;
  });

  // TXT: title, progress and the light band, no tiles.
  h.steps->push_back([h] {
    if (!writeTxtBook()) {
      LOG_ERR("SMOKE", "Picco screens: cannot write the TXT book");
      std::_Exit(2);
    }
    activityManager.goToReader(kTxtPath, true);
    return h.bookOpenTicks;
  });
  addSwipe(h, width / 2, height / 3, height * 2 / 3);
  h.steps->push_back([h] {
    h.expect("FrontlightPanel", "TXT swipe down");
    h.renderAndCapture("sheet-txt");
    return 0;
  });
  addTap(h, width / 2, height - 20);

  // Outside a book: header and light band from the top edge.
  h.steps->push_back([h] {
    activityManager.goToSettings();
    return h.settleTicks;
  });
  addSwipe(h, width / 2, 4, height / 2);
  h.steps->push_back([h] {
    h.expect("FrontlightPanel", "Settings top-edge swipe down");
    h.renderAndCapture("sheet-outside");
    return 0;
  });
  addTap(h, width / 2, height - 20);
  h.steps->push_back([h] {
    h.expect("Settings", "tap under the outside sheet");
    Storage.remove(kTxtPath);
    return 0;
  });
}

#else

void addPiccoSheetScreens(const PiccoScreenHooks&, const std::string&) {}

#endif
#endif
