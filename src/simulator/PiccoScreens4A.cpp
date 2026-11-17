#ifdef SIMULATOR
#include "PiccoScreens4A.h"

#include <Logging.h>

#include <cstdlib>
#include <tuple>

#include "AppCapabilities.h"
#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "activities/Activity.h"
#include "activities/ActivityManager.h"
#include "activities/browser/BookOrbitCatalogBrowserActivity.h"
#include "activities/network/CrossPointWebServerActivity.h"
#include "activities/network/UsbDriveActivity.h"
#include "activities/reader/BookStatsActivity.h"
#include "activities/util/KeyboardEntryActivity.h"

#if CROSSINK_APP_CAP_TOUCH && CROSSINK_APP_PICCO_UI
extern ActivityManager activityManager;
extern GfxRenderer renderer;
extern MappedInputManager mappedInputManager;

namespace {
constexpr int SETTLE_TICKS = 6;

using CatalogEntry = BookOrbitCatalogBrowserActivity::Entry;

CatalogEntry catalogBook(const char* title, const char* author, const char* meta, const bool onDevice) {
  CatalogEntry e{};
  e.type = BookOrbitCatalogBrowserActivity::EntryType::BOOK;
  e.title = title;
  e.subtitle = author;
  e.sectionId = meta;
  e.onDevice = onDevice;
  e.bookId = 1;
  if (onDevice) e.path = "/claude-test.epub";
  return e;
}

CatalogEntry catalogSection(const char* title) {
  CatalogEntry e{};
  e.type = BookOrbitCatalogBrowserActivity::EntryType::SECTION;
  e.title = title;
  e.sectionId = "recent";
  return e;
}

// Captures when the screen is up, else logs and goes on: a missed tap must not end the run.
void captureIf(const char* activity, const std::string& name,
               const std::function<void(const std::string&)>& renderAndCapture) {
  if (activityManager.isCurrentActivityNamed(activity)) {
    renderAndCapture(name);
    return;
  }
  LOG_ERR("SMOKE", "Picco screens 4A: %s skipped, %s is not up", name.c_str(), activity);
}

GlobalReadingStats statsFixture(const uint32_t scale) {
  GlobalReadingStats stats;
  stats.totalSessions = 312 * scale;
  stats.totalReadingSeconds = 128u * 3600u * scale;
  stats.totalPagesTurned = 9200 * scale;
  stats.completedBooks = 14 * scale;
  stats.timeOfDaySeconds = {14 * 3600u * scale, 21 * 3600u * scale, 48 * 3600u * scale, 17 * 3600u * scale};
  stats.dayOfWeekSeconds = {31 * 600u, 54 * 600u, 19 * 600u, 66 * 600u, 42 * 600u, 24 * 600u, 47 * 600u * scale};
  for (size_t i = 0; i < 6; ++i) stats.readingHistoryBits[i] = 0xB7;
  return stats;
}

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
  // Rocker: back to the root, then Up from the first row wraps onto the transfer pair.
  addPress(steps, MappedInputManager::Button::Back);
  addPress(steps, MappedInputManager::Button::Up);
  steps.push_back([renderAndCapture] {
    expectScreen("FileBrowser", "Files transfer focus");
    renderAndCapture("4a2-files-transfer-focus");
    return 0;
  });

  // BookOrbit catalog (T_BookOrbit, T_BookDetail): without credentials it opens on the
  // setup hint; the listings are injected (no server in this scenario).
  steps.push_back([] {
    activityManager.goToBookOrbitCatalog(false, false);
    return SETTLE_TICKS;
  });
  steps.push_back([renderAndCapture] {
    expectScreen("BookOrbitCatalogBrowser", "BookOrbit setup hint");
    renderAndCapture("4a2-bookorbit-error");
    return 0;
  });
  const auto showCatalog = [](const bool books, const bool detail) {
    return [books, detail] {
      auto* catalog = static_cast<BookOrbitCatalogBrowserActivity*>(activityManager.simulatorCurrentActivity());
      std::vector<CatalogEntry> rows;
      if (books) {
        rows.push_back(catalogBook("Walden", "Henry D. Thoreau", "Added yesterday", false));
        rows.push_back(catalogBook("Persuasion", "Jane Austen", "Added yesterday", false));
        rows.push_back(catalogBook("Middlemarch", "George Eliot", "Added 3 days ago", true));
        rows.push_back(catalogBook("North and South", "Elizabeth Gaskell", "Added 3 days ago", false));
        rows.push_back(catalogBook("The Mill on the Floss", "George Eliot", "Added last week", false));
        rows.push_back(catalogBook("Bleak House", "Charles Dickens", "Added last week", true));
      } else {
        for (const char* t :
             {"Continue reading (2)", "Recently added (14)", "Authors (31)", "Series (9)", "On device (6)", "Search"}) {
          rows.push_back(catalogSection(t));
        }
      }
      catalog->simulatorShowListing(std::move(rows), books ? 14 : 6, books, detail);
      return SETTLE_TICKS;
    };
  };
  for (const auto& [books, detail, name] :
       {std::make_tuple(false, false, "4a2-bookorbit-root"), std::make_tuple(true, false, "4a2-bookorbit-books"),
        std::make_tuple(true, true, "4a2-bookorbit-detail")}) {
    steps.push_back(showCatalog(books, detail));
    const std::string capture = name;
    steps.push_back([renderAndCapture, capture] {
      expectScreen("BookOrbitCatalogBrowser", capture.c_str());
      renderAndCapture(capture);
      return 0;
    });
  }

  // Sign-in form and keyboard (T_Login).
  steps.push_back([] {
    activityManager.goToBookOrbitAuth();
    return SETTLE_TICKS;
  });
  steps.push_back([renderAndCapture] {
    captureIf("BookOrbitAuth", "4a3-signin", renderAndCapture);
    return 0;
  });
  addTap(steps, 240, 318);  // the user name field
  steps.push_back([renderAndCapture] {
    captureIf("KeyboardEntry", "4a3-signin-keyboard", renderAndCapture);
    return 0;
  });
  addPress(steps, MappedInputManager::Button::Back);
  steps.push_back([] {
    activityManager.pushActivity(
        std::make_unique<KeyboardEntryActivity>(renderer, mappedInputManager, "Search", "persuasion", 64));
    return SETTLE_TICKS;
  });
  steps.push_back([renderAndCapture] {
    captureIf("KeyboardEntry", "4a3-keyboard", renderAndCapture);
    return 0;
  });

  // Reading stats (T_Stats): This device, then the All devices tab.
  steps.push_back([] {
    activityManager.replaceActivity(std::make_unique<BookStatsActivity>(renderer, mappedInputManager, "Reading stats",
                                                                        std::string{}, BookReadingStats{}, -1.0f, false,
                                                                        0, statsFixture(1), statsFixture(2)));
    return SETTLE_TICKS;
  });
  steps.push_back([renderAndCapture] {
    captureIf("BookStats", "4a3-stats", renderAndCapture);
    return 0;
  });
  addTap(steps, 360, 137);  // All devices tab
  steps.push_back([renderAndCapture] {
    captureIf("BookStats", "4a3-stats-all", renderAndCapture);
    return 0;
  });

  // Transfer screens: USB drive, the mode list, a running server (hotspot, joined network).
  steps.push_back([] {
    activityManager.replaceActivity(std::make_unique<UsbDriveActivity>(renderer, mappedInputManager));
    return SETTLE_TICKS;
  });
  steps.push_back([renderAndCapture] {
    captureIf("UsbDrive", "4a3-usb", renderAndCapture);
    return 0;
  });
  static CrossPointWebServerActivity* server = nullptr;
  steps.push_back([] {
    auto activity = std::make_unique<CrossPointWebServerActivity>(renderer, mappedInputManager);
    server = activity.get();
    activityManager.replaceActivity(std::move(activity));
    return SETTLE_TICKS;
  });
  steps.push_back([renderAndCapture] {
    captureIf("NetworkModeSelection", "4a3-transfer-modes", renderAndCapture);
    if (server) server->simulatorShowServer(true, "CrossPoint-Reader", "192.168.4.1");
    return 0;
  });
  addPress(steps, MappedInputManager::Button::Back);  // leaves the mode list; the fake server stays
  steps.push_back([renderAndCapture] {
    captureIf("CrossPointWebServer", "4a3-transfer-hotspot", renderAndCapture);
    if (server && activityManager.isCurrentActivityNamed("CrossPointWebServer")) {
      server->simulatorShowServer(false, "Home network", "192.168.1.42");
    }
    return SETTLE_TICKS;
  });
  steps.push_back([renderAndCapture] {
    captureIf("CrossPointWebServer", "4a3-transfer-wifi", renderAndCapture);
    server = nullptr;
    activityManager.goHome();
    return SETTLE_TICKS;
  });
}

#else

void addPiccoScreens4A(std::vector<std::function<int()>>&, const std::function<void(const std::string&)>&) {}

#endif
#endif
