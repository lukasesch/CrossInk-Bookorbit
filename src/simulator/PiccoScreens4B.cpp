#ifdef SIMULATOR
#include "PiccoScreens4B.h"

#include <I18n.h>
#include <Logging.h>

#include <cctype>
#include <cstdlib>

#include "AppCapabilities.h"
#include "activities/ActivityManager.h"
#include "activities/settings/SettingsActivity.h"
#include "activities/util/ConfirmationActivity.h"

#if CROSSINK_APP_CAP_TOUCH && CROSSINK_APP_PICCO_UI
extern ActivityManager activityManager;
extern GfxRenderer renderer;
extern MappedInputManager mappedInputManager;

struct PiccoScreens4B {
  using Capture = std::function<void(const std::string&)>;

  static SettingsActivity& settings() {
    auto* activity = dynamic_cast<SettingsActivity*>(activityManager.simulatorCurrentActivity());
    if (!activity) {
      LOG_ERR("SMOKE", "Picco screens 4B: Settings is not the current screen");
      std::_Exit(2);
    }
    return *activity;
  }

  static std::string slug(const char* text) {
    std::string out;
    for (const unsigned char c : std::string(text)) {
      if (std::isalnum(c)) {
        out.push_back(static_cast<char>(std::tolower(c)));
      } else if (!out.empty() && out.back() != '-') {
        out.push_back('-');
      }
    }
    while (!out.empty() && out.back() == '-') out.pop_back();
    return out;
  }

  // Every page of the current Picco settings list, without a focus frame.
  static void capturePages(SettingsActivity& screen, const Capture& capture, const std::string& name) {
    {
      RenderLock lock;
      screen.piccoFocus = false;
      screen.piccoFollowSelection = false;
      screen.piccoPage = 0;
    }
    capture(name);
    const int pages = screen.piccoPageCount;
    for (int page = 1; page < pages; ++page) {
      {
        RenderLock lock;
        screen.piccoPage = page;
      }
      capture(name + "-p" + std::to_string(page + 1));
    }
  }

  static void add(std::vector<std::function<int()>>& steps, const Capture& capture) {
    steps.push_back([] {
      activityManager.goToSettings();
      return 6;
    });
    steps.push_back([capture] {
      auto& screen = settings();
      capturePages(screen, capture, "settings-root");
      {
        // Rocker focus on the second row of the root.
        RenderLock lock;
        screen.piccoFocus = true;
        screen.piccoFollowSelection = true;
        screen.selectedSettingIndex = 3;
      }
      capture("settings-root-focus");

      const std::vector<SettingInfo> rows = screen.piccoRootSettings;
      for (const auto& row : rows) {
        bool opened = false;
        {
          RenderLock lock;
          opened = screen.openPiccoRootRow(row);
        }
        if (!opened) continue;
        capturePages(screen, capture, "settings-picco-" + slug(I18N.get(row.nameId)));
        RenderLock lock;
        screen.piccoBack();
      }

      // The full-page option picker, opened from Display & sleep > Refresh frequency.
      {
        RenderLock lock;
        screen.enterCategory(0);
        for (int i = 0; i < screen.settingsCount; ++i) {
          if ((*screen.currentSettings)[i].nameId == StrId::STR_REFRESH_FREQ) {
            screen.selectedSettingIndex = i + 1;
            screen.toggleCurrentSetting();
            break;
          }
        }
      }
      if (screen.piccoPicker.open) {
        capturePages(screen, capture, "settings-picker");
        RenderLock lock;
        screen.closePiccoPicker();
      } else {
        LOG_ERR("SMOKE", "Picco screens 4B: refresh frequency did not open the picker");
      }
      return 0;
    });

    // Confirmation dialog over the Settings page.
    steps.push_back([] {
      activityManager.pushActivity(std::make_unique<ConfirmationActivity>(
          renderer, mappedInputManager, tr(STR_RESET_ALL_TIME_STATS), tr(STR_RESET_ALL_TIME_STATS_CONFIRM)));
      return 6;
    });
    steps.push_back([capture] {
      capture("dialog");
      return 0;
    });
  }
};

void addPiccoScreens4B(std::vector<std::function<int()>>& steps,
                       const std::function<void(const std::string&)>& renderAndCapture) {
  PiccoScreens4B::add(steps, renderAndCapture);
}

#else

void addPiccoScreens4B(std::vector<std::function<int()>>&, const std::function<void(const std::string&)>&) {}

#endif
#endif
