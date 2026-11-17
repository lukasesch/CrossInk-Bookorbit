#ifdef SIMULATOR
#include "PiccoScreens4B.h"

#include <I18n.h>
#include <Logging.h>

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "AppCapabilities.h"
#include "CrossPointSettings.h"
#include "activities/ActivityManager.h"
#include "activities/network/WifiSelectionActivity.h"
#include "activities/settings/FontDownloadActivity.h"
#include "activities/settings/HyphenationManagerActivity.h"
#include "activities/settings/SettingsActivity.h"
#include "activities/settings/StatusBarSettingsActivity.h"
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
      screen.piccoList.focus = false;
      screen.piccoList.followSelection = false;
      screen.piccoList.page = 0;
    }
    capture(name);
    const int pages = screen.piccoList.pageCount;
    for (int page = 1; page < pages; ++page) {
      {
        RenderLock lock;
        screen.piccoList.page = page;
      }
      capture(name + "-p" + std::to_string(page + 1));
    }
  }

  template <typename T>
  static T& current(const char* what) {
    auto* activity = dynamic_cast<T*>(activityManager.simulatorCurrentActivity());
    if (!activity) {
      LOG_ERR("SMOKE", "Picco screens 4B: %s is not the current screen", what);
      std::_Exit(2);
    }
    return *activity;
  }

  // Every page of a PiccoListView screen, without a focus frame.
  static void captureViewPages(PiccoListView& view, const Capture& capture, const std::string& name) {
    {
      RenderLock lock;
      view.focus = false;
      view.followSelection = false;
      view.page = 0;
    }
    capture(name);
    const int pages = view.pageCount;
    for (int page = 1; page < pages; ++page) {
      {
        RenderLock lock;
        view.page = page;
      }
      capture(name + "-p" + std::to_string(page + 1));
    }
  }

  static WifiNetworkInfo network(const char* ssid, const int32_t rssi, const bool encrypted, const bool saved) {
    WifiNetworkInfo n;
    n.ssid = ssid;
    n.rssi = rssi;
    n.isEncrypted = encrypted;
    n.hasSavedPassword = saved;
    return n;
  }

  // T_Wifi(2), T_HyphenationPacks(2), T_Fonts, T_StatusBarSettings(2) on seeded data.
  static void addSystemScreens(std::vector<std::function<int()>>& steps, const Capture& capture) {
    steps.push_back([] {
      activityManager.pushActivity(std::make_unique<WifiSelectionActivity>(renderer, mappedInputManager, false));
      return 6;
    });
    steps.push_back([capture] {
      auto& wifi = current<WifiSelectionActivity>("Wi-Fi");
      {
        RenderLock lock;
        wifi.networks = {network("Home 2.4G", -48, true, true),    network("Office", -63, true, false),
                         network("Guest", -75, false, false),      network("Library", -58, true, false),
                         network("Cafe Lumen", -71, false, false), network("Neighbour", -82, true, false),
                         network("Studio 5G", -66, true, false)};
        wifi.realNetworkCount = wifi.networks.size();
        wifi.appendHiddenNetworkEntry();
        wifi.state = WifiSelectionState::NETWORK_LIST;
        wifi.piccoList.reset();
        wifi.piccoSelected = 1;
      }
      captureViewPages(wifi.piccoList, capture, "wifi");
      {
        RenderLock lock;
        wifi.piccoList.focus = true;
        wifi.piccoList.followSelection = true;
        wifi.piccoSelected = 2;
      }
      capture("wifi-focus");
      activityManager.popActivity();
      return 6;
    });

    steps.push_back([] {
      activityManager.pushActivity(std::make_unique<HyphenationManagerActivity>(renderer, mappedInputManager));
      return 6;
    });
    steps.push_back([capture] {
      auto& hyphenation = current<HyphenationManagerActivity>("Hyphenation");
      captureViewPages(hyphenation.piccoList_, capture, "hyphenation");
      {
        RenderLock lock;
        hyphenation.selected_ = 1;
        hyphenation.piccoActions_ = true;
        hyphenation.piccoActionRow_ = 0;
        hyphenation.piccoList_.reset();
      }
      capture("hyphenation-actions");
      activityManager.popActivity();
      return 6;
    });

    steps.push_back([] {
      auto fonts = std::make_unique<FontDownloadActivity>(renderer, mappedInputManager);
      fonts->simulatorOffline_ = true;
      struct Seed {
        const char* name;
        const char* description;
        bool installed;
        bool update;
      };
      static constexpr Seed kSeed[] = {{"Literata", "Chapter 41", true, false},
                                       {"Atkinson Hyperlegible", "Chapter 41", true, true},
                                       {"EB Garamond", "Chapter 41", false, false},
                                       {"Merriweather", "Chapter 41", false, false}};
      constexpr size_t kCount = sizeof(kSeed) / sizeof(kSeed[0]);
      fonts->manifestFamilies_ = std::make_unique<FontDownloadActivity::ManifestFamily[]>(kCount);
      for (size_t i = 0; i < kCount; ++i) {
        auto& family = fonts->manifestFamilies_[i];
        family.name = kSeed[i].name;
        family.installName = kSeed[i].name;
        family.description = kSeed[i].description;
        family.installed = kSeed[i].installed;
        family.hasUpdate = kSeed[i].update;
      }
      fonts->manifestFamilyCount_ = kCount;
      fonts->state_ = FontDownloadActivity::FAMILY_LIST;
      std::snprintf(fonts->updateAllLabel_, sizeof(fonts->updateAllLabel_), "%s (1.2 MB)", tr(STR_UPDATE_ALL));
      std::strncpy(SETTINGS.sdFontFamilyName, "Literata", sizeof(SETTINGS.sdFontFamilyName) - 1);
      activityManager.pushActivity(std::move(fonts));
      return 6;
    });
    steps.push_back([capture] {
      auto& fonts = current<FontDownloadActivity>("Fonts");
      captureViewPages(fonts.piccoList_, capture, "fonts");
      SETTINGS.sdFontFamilyName[0] = '\0';
      activityManager.popActivity();
      return 6;
    });

    steps.push_back([] {
      activityManager.pushActivity(std::make_unique<StatusBarSettingsActivity>(renderer, mappedInputManager));
      return 6;
    });
    steps.push_back([capture] {
      auto& bar = current<StatusBarSettingsActivity>("Status bar");
      captureViewPages(bar.piccoList, capture, "statusbar-root");
      {
        RenderLock lock;
        bar.selectedIndex = 1;  // Bottom bar
        bar.handleSelection();
        bar.piccoList.reset();
      }
      captureViewPages(bar.piccoList, capture, "statusbar-bottom");
      {
        RenderLock lock;
        bar.selectedIndex = 0;  // Left 1: the item picker
        bar.openOptionPicker();
      }
      capture("statusbar-picker");
      {
        RenderLock lock;
        bar.optionPopup.clear();
      }
      activityManager.popActivity();
      return 6;
    });
  }

  static void add(std::vector<std::function<int()>>& steps, const Capture& capture) {
    addSystemScreens(steps, capture);
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
        screen.piccoList.focus = true;
        screen.piccoList.followSelection = true;
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
      // Sleep after: the stepper row's Confirm opens the ladder picker; Language uses it too.
      {
        RenderLock lock;
        screen.openPiccoSleepPicker();
      }
      capturePages(screen, capture, "settings-picker-sleep");
      {
        RenderLock lock;
        screen.closePiccoPicker();
      }
      screen.openPiccoLanguagePicker();
      capturePages(screen, capture, "settings-picker-language");
      {
        RenderLock lock;
        screen.closePiccoPicker();
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
