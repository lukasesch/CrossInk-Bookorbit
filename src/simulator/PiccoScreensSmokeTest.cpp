#ifdef SIMULATOR
#include "PiccoScreensSmokeTest.h"

#include <HalDisplay.h>
#include <I18n.h>
#include <Logging.h>

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "AppCapabilities.h"
#include "BookOrbitCredentialStore.h"
#include "CrossPointSettings.h"
#include "PiccoScreens3B.h"
#include "PiccoScreens4A.h"
#include "PiccoScreens4B.h"
#include "PiccoScreens4R1.h"
#include "PiccoScreens4R2.h"
#include "activities/ActivityManager.h"
#include "activities/boot_sleep/SleepActivity.h"
#include "activities/home/HomeActivity.h"
#include "activities/reader/ReadingDailyMinutes.h"
#include "activities/reader/ReadingStatsUtils.h"
#include "activities/settings/SettingsActivity.h"
#include "components/UITheme.h"

#if CROSSINK_APP_CAP_TOUCH
extern ActivityManager activityManager;
extern GfxRenderer renderer;
extern MappedInputManager mappedInputManager;

// Walks the app through its screens and writes one PGM per screen, named after it.
// Navigation uses the app's own routes and, where the device uses one, the touch
// gesture (reader light panel and drawer, Home light panel). Captures are the
// logical framebuffer, so they show what the panel would show.
struct PiccoScreensSmokeTest {
  // Returns the number of main-loop ticks to wait before the next step.
  using Step = std::function<int()>;

  static constexpr int SETTLE_TICKS = 6;
  static constexpr int BOOK_OPEN_TICKS = 40;

  std::vector<Step> steps;
  size_t next = 0;
  int wait = 0;
  int captured = 0;

  static const char* outputDir() { return std::getenv("CROSSINK_SIMULATOR_SMOKE_PICCO_SCREENS"); }

  [[noreturn]] static void fail(const char* message) {
    LOG_ERR("SMOKE", "Picco screens: %s", message);
    std::_Exit(2);
  }

  static std::string slug(const std::string& text) {
    std::string out;
    for (const unsigned char c : text) {
      if (std::isalnum(c)) {
        out.push_back(static_cast<char>(std::tolower(c)));
      } else if (!out.empty() && out.back() != '-') {
        out.push_back('-');
      }
    }
    while (!out.empty() && out.back() == '-') out.pop_back();
    return out;
  }

  static std::string currentName() {
    const Activity* activity = activityManager.simulatorCurrentActivity();
    if (!activity) return "none";
    for (const char* name : {"Home", "EpubReader", "EpubReaderDrawer", "FrontlightPanel", "Library", "FileBrowser",
                             "Settings", "BookStats", "BookOrbitCatalogBrowser"}) {
      if (activityManager.isCurrentActivityNamed(name)) return name;
    }
    return "other";
  }

  static void render() {
    if (activityManager.requestUpdateAndWait() != RequestUpdateResult::Rendered) fail("render was rejected");
  }

  void capture(const std::string& name) {
    RenderLock lock;
    std::filesystem::create_directories(outputDir());
    const auto path = std::filesystem::path(outputDir()) / (name + ".pgm");
    FILE* image = std::fopen(path.c_str(), "wb");
    if (!image) fail("cannot write capture");
    const int width = renderer.getScreenWidth();
    const int height = renderer.getScreenHeight();
    const bool inverted = display.isInverted();
    std::fprintf(image, "P5\n%d %d\n255\n", width, height);
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) std::fputc(renderer.isPixelBlack(x, y) != inverted ? 0 : 255, image);
    }
    if (std::fclose(image) != 0) fail("cannot save capture");
    ++captured;
    LOG_INF("SMOKE", "Picco screen captured: %s (%s)", name.c_str(), currentName().c_str());
  }

  void renderAndCapture(const std::string& name) {
    render();
    capture(name);
  }

  void expect(const char* activity, const char* what) {
    if (activityManager.isCurrentActivityNamed(activity)) return;
    LOG_ERR("SMOKE", "Picco screens: %s expected %s, found %s", what, activity, currentName().c_str());
    std::_Exit(2);
  }

  // A vertical swipe, one touch phase per tick as the main loop sees a finger.
  void addSwipe(const int x, const int fromY, const int toY) {
    steps.push_back([x, fromY] {
      mappedInputManager.simulatorInjectTouchDown(x, fromY);
      return 0;
    });
    steps.push_back([x, toY] {
      mappedInputManager.simulatorInjectTouchMove(x, toY);
      return 0;
    });
    steps.push_back([x, toY] {
      mappedInputManager.simulatorInjectTouchRelease(x, toY);
      return SETTLE_TICKS;
    });
  }

  void addTap(const int x, const int y) {
    steps.push_back([x, y] {
      mappedInputManager.simulatorInjectTouchDown(x, y);
      return 0;
    });
    steps.push_back([x, y] {
      mappedInputManager.simulatorInjectTouchRelease(x, y);
      return SETTLE_TICKS;
    });
  }

  void addPress(const MappedInputManager::Button button) {
    steps.push_back([button] {
      mappedInputManager.simulatorInjectPress(button);
      return 0;
    });
    steps.push_back([button] {
      mappedInputManager.simulatorInjectRelease(button);
      return SETTLE_TICKS;
    });
  }

  // A week of reading for Home's stats card, today last (minutes as on the T_Combined board).
  static void seedReadingWeek() {
    ReadingStatsDateTime now;
    if (!getCurrentLocalReadingStatsDateTimeAnyClock(now)) {
      LOG_INF("SMOKE", "Picco screens: no clock, Home stats stay empty");
      return;
    }
    static constexpr uint32_t kMinutes[7] = {40, 70, 25, 85, 55, 30, 47};
    for (int i = 0; i < 7; ++i) {
      ReadingStatsDateTime start = now;
      addDaysToReadingStatsDate(start.date, i - 6);
      start.hour = 6;
      start.minute = 0;
      start.second = 0;
      ReadingDailyMinutes::record(&start, kMinutes[i] * 60u);
    }
  }

  void addBack() {
    steps.push_back([] {
      mappedInputManager.simulatorInjectPress(MappedInputManager::Button::Back);
      return 0;
    });
    steps.push_back([] {
      mappedInputManager.simulatorInjectRelease(MappedInputManager::Button::Back);
      return SETTLE_TICKS;
    });
  }

  static SettingsActivity& settings() {
    auto* activity = dynamic_cast<SettingsActivity*>(activityManager.simulatorCurrentActivity());
    if (!activity) fail("Settings is not the current screen");
    return *activity;
  }

  // The visible page plus one capture per further page of a long list.
  void captureSettingsPages(const std::string& name) {
    auto& screen = settings();
    renderAndCapture(name);
    const int rows = std::max(1, screen.visibleRows);
    for (int page = 1; page * rows < screen.settingsCount; ++page) {
      {
        RenderLock lock;
        screen.topIndex = page * rows;
        screen.selectedSettingIndex = page * rows + 1;
      }
      renderAndCapture(name + "-p" + std::to_string(page + 1));
    }
    RenderLock lock;
    screen.topIndex = 0;
    screen.selectedSettingIndex = 1;
  }

  void captureSubmenus(const std::string& prefix, const int depth) {
    auto& screen = settings();
    std::vector<std::pair<SettingAction, std::string>> submenus;
    for (const auto& setting : *screen.currentSettings) {
      if (setting.type == SettingType::SUBMENU) submenus.emplace_back(setting.action, I18N.get(setting.nameId));
    }
    for (const auto& [action, label] : submenus) {
      {
        RenderLock lock;
        screen.openSubmenu(action);
      }
      const std::string name = prefix + "-" + slug(label);
      captureSettingsPages(name);
      if (depth < 2) captureSubmenus(name, depth + 1);
      RenderLock lock;
      screen.closeSubmenu();
    }
  }

  void build() {
    const char* book = std::getenv("CROSSINK_SIMULATOR_SMOKE_BOOK");
    if (!book) fail("CROSSINK_SIMULATOR_SMOKE_BOOK is not set");
    const std::string bookPath = book;
    const int width = renderer.getScreenWidth();
    const int height = renderer.getScreenHeight();

    steps.push_back([] {
#if CROSSINK_APP_PICCO_UI
      // The captures are the Picco UI's visual gate: render every screen in the Picco theme.
      {
        RenderLock lock;
        SETTINGS.uiTheme = CrossPointSettings::UI_THEME::PICCO;
        UITheme::getInstance().reload();
      }
#endif
      activityManager.goHome();
      return SETTLE_TICKS;
    });
    steps.push_back([this] {
      expect("Home", "boot");
      renderAndCapture("home-empty");
      return 0;
    });

    // Reader: a page, then the light panel (swipe down) and the reader drawer (swipe up).
    steps.push_back([bookPath] {
      activityManager.goToReader(bookPath, true);
      return BOOK_OPEN_TICKS;
    });
    steps.push_back([this] {
      expect("EpubReader", "open book");
      renderAndCapture("reader");
      return 0;
    });
    addSwipe(width / 2, height / 3, height * 2 / 3);
    steps.push_back([this] {
      expect("FrontlightPanel", "reader swipe down");
      renderAndCapture("reader-light-panel");
      return 0;
    });
    addTap(width / 2, height - 40);
    steps.push_back([this] {
      expect("EpubReader", "light panel dismissal");
      return 0;
    });
    // Under the Picco UI the sheet replaces the drawer; swipe up goes Home (PiccoScreens3B).
    addPiccoSheetScreens(
        {&steps, [this](const std::string& name) { renderAndCapture(name); },
         [this](const char* activity, const char* what) { expect(activity, what); }, SETTLE_TICKS, BOOK_OPEN_TICKS},
        bookPath);

    // Home with the book just read, and its light panel (top-edge swipe down).
    steps.push_back([] {
      seedReadingWeek();
      activityManager.goHome();
      return SETTLE_TICKS;
    });
    steps.push_back([this] {
      expect("Home", "go Home");
      renderAndCapture("home");
      return 0;
    });
    // Rocker focus: the first press shows the ring on Resume, the second moves it on.
    addPress(MappedInputManager::Button::Down);
    addPress(MappedInputManager::Button::Down);
    steps.push_back([this] {
      expect("Home", "Home focus");
      renderAndCapture("home-focus-stats");
      return 0;
    });
    addSwipe(width / 2, 4, height / 2);
    steps.push_back([this] {
      expect("FrontlightPanel", "Home top-edge swipe down");
      renderAndCapture("home-light-panel");
      return 0;
    });
    addTap(width / 2, height - 40);

    steps.push_back([] {
      activityManager.goHome();
      return SETTLE_TICKS;
    });
    steps.push_back([] {
      auto* home = dynamic_cast<HomeActivity*>(activityManager.simulatorCurrentActivity());
      if (!home) fail("Home is not the current screen");
      auto stats = home->createFrontlightReadingStatsActivity();
      if (!stats) {
        LOG_INF("SMOKE", "Picco screen skipped: reading-stats (reading stats are off)");
        return 0;
      }
      activityManager.pushActivity(std::move(stats));
      return SETTLE_TICKS;
    });
    steps.push_back([this] {
      if (activityManager.isCurrentActivityNamed("BookStats")) renderAndCapture("reading-stats");
      return 0;
    });

    steps.push_back([] {
      activityManager.goToLibrary();
      return SETTLE_TICKS * 2;
    });
    steps.push_back([this] {
      expect("Library", "go to Library");
      renderAndCapture("library");
      return 0;
    });

    steps.push_back([] {
      activityManager.goToFileBrowser();
      return SETTLE_TICKS;
    });
    steps.push_back([this] {
      expect("FileBrowser", "go to File browser");
      renderAndCapture("file-browser");
      return 0;
    });

    addPiccoScreens4A(steps, [this](const std::string& name) { renderAndCapture(name); });

    steps.push_back([] {
      activityManager.goToSettings();
      return SETTLE_TICKS;
    });
    steps.push_back([this] {
      expect("Settings", "go to Settings");
      auto& screen = settings();
      for (int category = 0; category < SettingsActivity::categoryCount; ++category) {
        {
          RenderLock lock;
          screen.enterCategory(category);
          screen.selectedSettingIndex = 1;
          screen.topIndex = 0;
        }
        const std::string name = "settings-" + slug(I18N.get(SettingsActivity::categoryNames[category]));
        captureSettingsPages(name);
        captureSubmenus(name, 1);
      }
      return 0;
    });

    addPiccoScreens4B(steps, [this](const std::string& name) { renderAndCapture(name); });
    addPiccoScreens4R2(
        steps, [this](const std::string& name) { renderAndCapture(name); },
        [this](const std::string& name) { capture(name); }, bookPath);
    addPiccoScreens4R1(
        steps, bookPath, [this](const std::string& name) { capture(name); },
        [this](const std::string& name) { renderAndCapture(name); });

    // Without an account the catalog shows its setup hint and makes no request.
    steps.push_back([] {
      if (!activityManager.goToBookOrbitCatalog()) fail("BookOrbit catalog did not open");
      return SETTLE_TICKS * 2;
    });
    steps.push_back([this] {
      expect("BookOrbitCatalogBrowser", "go to BookOrbit catalog");
      renderAndCapture("bookorbit-catalog");
      return 0;
    });

    // Home with a BookOrbit account (never synced): the header's sync button and tile hint.
    steps.push_back([] {
      BOOKORBIT_STORE.setServerUrl("http://bookorbit.invalid");
      BOOKORBIT_STORE.setCredentials("reader", "smoke");
      activityManager.goHome();
      return SETTLE_TICKS;
    });
    steps.push_back([this] {
      expect("Home", "Home with BookOrbit");
      renderAndCapture("home-bookorbit");
      BOOKORBIT_STORE.clearCredentials();
      return 0;
    });

    // The sleep screen draws in onEnter; a detached instance keeps the app awake.
    steps.push_back([this] {
      SleepActivity sleep(renderer, mappedInputManager, false);
      sleep.onEnter();
      capture("sleep");
      return 0;
    });
  }

  bool tick() {
    if (!outputDir()) return false;
    if (steps.empty()) build();
    if (wait > 0) {
      --wait;
      return true;
    }
    if (next >= steps.size()) {
      LOG_INF("SMOKE", "Simulator smoke test passed: picco screens (%d captures)", captured);
      std::_Exit(0);
    }
    wait = steps[next++]();
    return true;
  }
};

namespace {
PiccoScreensSmokeTest piccoScreens;
}

bool tickPiccoScreensSmokeTest() { return piccoScreens.tick(); }

#else

// The scenario reaches the light panel and reader drawer by touch.
bool tickPiccoScreensSmokeTest() {
  if (!std::getenv("CROSSINK_SIMULATOR_SMOKE_PICCO_SCREENS")) return false;
  LOG_ERR("SMOKE", "Picco screens need a touch simulator (picco-simulator)");
  std::_Exit(2);
}

#endif

#endif
