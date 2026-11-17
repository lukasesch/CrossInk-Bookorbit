#ifdef SIMULATOR
#include "PiccoScreens4A.h"

#include <Logging.h>

#include <cstdlib>

#include "AppCapabilities.h"
#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "activities/Activity.h"
#include "activities/ActivityManager.h"

#if CROSSINK_APP_CAP_TOUCH && CROSSINK_APP_PICCO_UI
extern ActivityManager activityManager;
extern MappedInputManager mappedInputManager;

namespace {
constexpr int SETTLE_TICKS = 6;

void expectScreen(const char* activity, const char* what) {
  if (activityManager.isCurrentActivityNamed(activity)) return;
  LOG_ERR("SMOKE", "Picco screens 4A: %s expected %s", what, activity);
  std::_Exit(2);
}

void addTap(std::vector<std::function<int()>>& steps, const int x, const int y) {
  steps.push_back([x, y] {
    mappedInputManager.simulatorInjectTouchDown(x, y);
    return 0;
  });
  steps.push_back([x, y] {
    mappedInputManager.simulatorInjectTouchRelease(x, y);
    return SETTLE_TICKS;
  });
}

void addPress(std::vector<std::function<int()>>& steps, const MappedInputManager::Button button) {
  steps.push_back([button] {
    mappedInputManager.simulatorInjectPress(button);
    return 0;
  });
  steps.push_back([button] {
    mappedInputManager.simulatorInjectRelease(button);
    return SETTLE_TICKS;
  });
}
}  // namespace

void addPiccoScreens4A(std::vector<std::function<int()>>& steps,
                       const std::function<void(const std::string&)>& renderAndCapture) {
  // Library: cover grid, list, sort screen, Saved tab (T_Library, T_LibrarySort, T_LibrarySaved).
  steps.push_back([] {
    SETTINGS.recentBooksView = CrossPointSettings::RECENT_BOOKS_GRID;
    activityManager.goToLibrary();
    return SETTLE_TICKS * 2;
  });
  steps.push_back([renderAndCapture] {
    expectScreen("Library", "Library grid");
    renderAndCapture("4a-library-grid");
    return 0;
  });
  steps.push_back([] {
    SETTINGS.recentBooksView = CrossPointSettings::RECENT_BOOKS_LIST;
    activityManager.goToLibrary();
    return SETTLE_TICKS * 2;
  });
  steps.push_back([renderAndCapture] {
    expectScreen("Library", "Library list");
    renderAndCapture("4a-library-list");
    return 0;
  });
  addTap(steps, 80, 208);  // sort dropdown
  steps.push_back([renderAndCapture] {
    expectScreen("Library", "Library sort");
    renderAndCapture("4a-library-sort");
    return 0;
  });
  addPress(steps, MappedInputManager::Button::Back);
  addTap(steps, 340, 128);  // Saved tab
  steps.push_back([renderAndCapture] {
    expectScreen("SavedItemsHome", "Saved tab");
    renderAndCapture("4a-library-saved");
    return 0;
  });

  // Files (T_Files).
  steps.push_back([] {
    activityManager.goToFileBrowser();
    return SETTLE_TICKS;
  });
  steps.push_back([renderAndCapture] {
    expectScreen("FileBrowser", "Files");
    renderAndCapture("4a-files");
    return 0;
  });
  addTap(steps, 200, 204);  // first row (the books folder)
  steps.push_back([renderAndCapture] {
    expectScreen("FileBrowser", "Files subfolder");
    renderAndCapture("4a-files-folder");
    return 0;
  });
}

#else

void addPiccoScreens4A(std::vector<std::function<int()>>&, const std::function<void(const std::string&)>&) {}

#endif
#endif
