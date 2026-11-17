// Picco Settings (T_Settings, T_Settings2): grouped root and hand-drawn paged rows. The
// category pages reuse SettingsActivity's lists, so every option stays where it was below
// the root; only the four tabs are replaced.
#include "SettingsActivity.h"

#if CROSSINK_APP_PICCO_UI

#include <CrossInkHalFrontlight.h>
#include <GfxRenderer.h>
#include <I18n.h>
#include <Logging.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

#include "AppVersion.h"
#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "SettingsList.h"
#include "components/CompactHeader.h"
#include "components/TouchHeaderBackButton.h"
#include "components/TouchRegistry.h"
#include "components/UITheme.h"
#include "components/picco/PiccoPageIndicator.h"
#include "components/picco/PiccoSettingsRows.h"
#include "components/picco/PiccoValueSteps.h"
#include "fontIds.h"
#include "util/PluginEvents.h"
#include "util/SlideSwitch.h"

namespace {
using PiccoSettingsRows::Row;
using PiccoSettingsRows::Trailing;

bool isConnectionAction(const SettingAction action) {
  switch (action) {
    case SettingAction::Network:
    case SettingAction::KOReaderSync:
    case SettingAction::BookOrbitSync:
    case SettingAction::OPDSBrowser:
      return true;
    default:
      return false;
  }
}

StrId categoryTitle(const int category) {
  switch (category) {
    case 0:
      return StrId::STR_PICCO_DISPLAY_SLEEP;
    case 1:
      return StrId::STR_PICCO_TEXT_FONTS;
    case 2:
      return StrId::STR_CAT_CONTROLS;
    default:
      return StrId::STR_CAT_SYSTEM;
  }
}

int categoryFor(const SettingAction action) {
  switch (action) {
    case SettingAction::PiccoDisplay:
      return 0;
    case SettingAction::PiccoReading:
      return 1;
    case SettingAction::PiccoControls:
      return 2;
    case SettingAction::PiccoSystem:
      return 3;
    default:
      return -1;
  }
}

SettingAction rootActionFor(const int category) {
  switch (category) {
    case 0:
      return SettingAction::PiccoDisplay;
    case 1:
      return SettingAction::PiccoReading;
    case 2:
      return SettingAction::PiccoControls;
    default:
      return SettingAction::PiccoSystem;
  }
}

bool copyByName(std::vector<SettingInfo>& target, const std::vector<SettingInfo>& source, const StrId nameId) {
  const auto it =
      std::find_if(source.begin(), source.end(), [nameId](const SettingInfo& s) { return s.nameId == nameId; });
  if (it == source.end()) return false;
  target.push_back(*it);
  return true;
}

void eraseByName(std::vector<SettingInfo>& list, const StrId nameId) {
  list.erase(std::remove_if(list.begin(), list.end(), [nameId](const SettingInfo& s) { return s.nameId == nameId; }),
             list.end());
}

// Second line under a row's label (T_SettingsDisplay, T_SettingsLight2, T_Controls, T_ControlsTilt,
// T_SettingsSystem); nullptr when the board shows none.
const char* piccoHint(const SettingInfo& setting) {
  switch (setting.nameId) {
    case StrId::STR_QUICK_RESUME_TIMEOUT:
      return tr(STR_PICCO_HINT_QUICK_RESUME);
    case StrId::STR_REFRESH_FREQ:
      return tr(STR_PICCO_HINT_FULL_REFRESH);
    case StrId::STR_RESTORE_LIGHT_ON_WAKE:
      return tr(STR_PICCO_HINT_RESTORE_LIGHT);
    case StrId::STR_KEEP_CLOCK_IN_SLEEP:
      return tr(STR_PICCO_HINT_KEEP_CLOCK);
    case StrId::STR_CUSTOM_BOOTSCREEN:
      return tr(STR_PICCO_HINT_BOOT_SCREEN);
    case StrId::STR_DISABLE_TOUCHSCREEN:
      return tr(STR_PICCO_HINT_TOUCHSCREEN);
    case StrId::STR_TILT_PAGE_TURN:
      return tr(STR_PICCO_HINT_TILT);
    case StrId::STR_TILT_NEXT_PAGE_ONLY:
      return tr(STR_PICCO_HINT_TILT_NEXT_ONLY);
    case StrId::STR_PWR_BTN_FOOTNOTE_BACK:
      return tr(STR_PICCO_HINT_FOOTNOTE_BACK);
    default:
      return nullptr;
  }
}

// Rows with an inline minus / value / plus stepper (T_SettingsDisplay "Sleep after", "Full refresh").
bool isPiccoStepper(const SettingInfo& setting) {
  return setting.nameId == StrId::STR_TIME_TO_SLEEP || setting.nameId == StrId::STR_REFRESH_FREQ;
}

// Touch ids of a stepper row's buttons, above every row index.
constexpr int kStepperIdBase = 1000;
constexpr int stepperId(const int row, const bool plus) { return kStepperIdBase + 2 * row + (plus ? 1 : 0); }

void formatSleepMinutes(const unsigned minutes, char* out, const size_t len) {
  if (minutes >= CrossPointSettings::SLEEP_TIMEOUT_NEVER_MINUTES) {
    std::snprintf(out, len, "%s", tr(STR_NEVER));
  } else {
    std::snprintf(out, len, tr(STR_SLEEP_TIMER_VALUE_FORMAT), minutes);
  }
}

int enumIndexOf(const SettingInfo& setting, const uint8_t raw) {
  if (setting.enumRawValues.empty()) return raw;
  const auto it = std::find(setting.enumRawValues.begin(), setting.enumRawValues.end(), raw);
  return it == setting.enumRawValues.end() ? 0 : static_cast<int>(it - setting.enumRawValues.begin());
}

uint8_t enumRawAt(const SettingInfo& setting, const int index) {
  if (setting.enumRawValues.empty()) return static_cast<uint8_t>(index);
  return setting.enumRawValues[std::clamp<int>(index, 0, static_cast<int>(setting.enumRawValues.size()) - 1)];
}

bool opensSomething(const SettingInfo& setting) {
  return setting.type == SettingType::SUBMENU || setting.type == SettingType::ACTION ||
         setting.type == SettingType::STRING || setting.type == SettingType::VALUE;
}
}  // namespace

bool SettingsActivity::usesPiccoSettings() const { return UITheme::isPicco() && !isFileBrowserView(); }

void SettingsActivity::rebuildPiccoLists(const std::vector<SettingInfo>& allSettings) {
  // Connections own Wi-Fi and the sync services on Picco; System keeps the rest.
  systemSettings.erase(std::remove_if(systemSettings.begin(), systemSettings.end(),
                                      [](const SettingInfo& s) { return isConnectionAction(s.action); }),
                       systemSettings.end());
  reflowPiccoDisplay(allSettings);
  reflowPiccoLight(allSettings);
  reflowPiccoControls(allSettings);
  reflowPiccoSystem();

  piccoRootSettings.clear();
  piccoRootSettings.reserve(14);
  piccoRootSettings.push_back(SettingInfo::SectionHeader(StrId::STR_PICCO_SETTINGS_READING));
  piccoRootSettings.push_back(SettingInfo::Submenu(StrId::STR_PICCO_TEXT_FONTS, SettingAction::PiccoReading));
  piccoRootSettings.push_back(SettingInfo::Action(StrId::STR_HYPHENATION, SettingAction::ManageHyphenation));
  addSettingByName(piccoRootSettings, allSettings, StrId::STR_DICTIONARY);
  piccoRootSettings.push_back(SettingInfo::SectionHeader(StrId::STR_SYSTEM_DEVICE));
  piccoRootSettings.push_back(SettingInfo::Submenu(StrId::STR_PICCO_DISPLAY_SLEEP, SettingAction::PiccoDisplay));
  if (Frontlight.present()) {
    piccoRootSettings.push_back(SettingInfo::Submenu(StrId::STR_LIGHT, SettingAction::DisplayFrontlight));
  }
  piccoRootSettings.push_back(SettingInfo::Submenu(StrId::STR_CAT_CONTROLS, SettingAction::PiccoControls));
  piccoRootSettings.push_back(SettingInfo::Submenu(StrId::STR_CAT_SYSTEM, SettingAction::PiccoSystem));
  piccoRootSettings.push_back(SettingInfo::SectionHeader(StrId::STR_PICCO_SETTINGS_CONNECTIONS));
  piccoRootSettings.push_back(SettingInfo::Action(StrId::STR_PICCO_WIFI, SettingAction::Network));
  piccoRootSettings.push_back(SettingInfo::Action(StrId::STR_BOOKORBIT, SettingAction::BookOrbitSync));
#if CROSSINK_APP_CAP_KOREADER_SYNC
  piccoRootSettings.push_back(SettingInfo::Action(StrId::STR_PICCO_KOREADER_SYNC, SettingAction::KOReaderSync));
#endif
#if CROSSINK_APP_CAP_OPDS
  piccoRootSettings.push_back(SettingInfo::Action(StrId::STR_PICCO_OPDS_CATALOGS, SettingAction::OPDSBrowser));
#endif
}

// T_SettingsDisplay(2): Home, Sleep, Refresh, Screen. Frontlight moves to the root's Light row;
// Sleep after comes over from System > Device and Quick resume from the sleep-screen page.
void SettingsActivity::reflowPiccoDisplay(const std::vector<SettingInfo>& allSettings) {
  const std::vector<SettingInfo> upstream = std::move(displaySettings);
  displaySettings.clear();
  displaySettings.reserve(upstream.size() + 6);
  std::vector<SettingInfo>& out = displaySettings;
  out.push_back(SettingInfo::SectionHeader(StrId::STR_PICCO_SECTION_HOME));
  copyByName(out, upstream, StrId::STR_UI_THEME);
  copyByName(out, upstream, StrId::STR_SWAP_LIBRARY_FILE_BROWSER);
  addSettingByName(out, allSettings, StrId::STR_QUICK_RESUME_TIMEOUT);
  out.push_back(SettingInfo::SectionHeader(StrId::STR_PICCO_SECTION_SLEEP));
  out.push_back(SettingInfo::Submenu(StrId::STR_SLEEP_SCREEN, SettingAction::DisplaySleepScreen));
  addSettingByName(out, allSettings, StrId::STR_TIME_TO_SLEEP);
  out.push_back(SettingInfo::SectionHeader(StrId::STR_PICCO_SECTION_REFRESH));
  copyByName(out, upstream, StrId::STR_REFRESH_FREQ);
  copyByName(out, upstream, StrId::STR_SUNLIGHT_FADING_FIX);
  copyByName(out, upstream, StrId::STR_READER_DARK_MODE);
  out.push_back(SettingInfo::SectionHeader(StrId::STR_PICCO_SECTION_SCREEN));
  copyByName(out, upstream, StrId::STR_UI_SCALE);
  out.push_back(SettingInfo::Action(StrId::STR_STATUS_BAR, SettingAction::DisplayStatusBar));
  out.push_back(SettingInfo::Action(StrId::STR_SCREEN_CALIBRATION, SettingAction::ScreenCalibration));
  eraseByName(displaySleepSettings, StrId::STR_QUICK_RESUME_TIMEOUT);
}

// T_SettingsLight(2): Default, Slide switch, Schedule, Wake.
void SettingsActivity::reflowPiccoLight(const std::vector<SettingInfo>& allSettings) {
  if (!Frontlight.present()) return;
  const std::vector<SettingInfo> upstream = std::move(displayFrontlightSettings);
  displayFrontlightSettings.clear();
  displayFrontlightSettings.reserve(upstream.size() + 5);
  std::vector<SettingInfo>& out = displayFrontlightSettings;
  out.push_back(SettingInfo::SectionHeader(StrId::STR_PICCO_SECTION_DEFAULT));
  for (const auto& s : upstream) {
    if (s.action == SettingAction::FrontlightPanel) out.push_back(s);
  }
  if (deviceHasSlideSwitch(gpio)) {
    out.push_back(SettingInfo::SectionHeader(StrId::STR_PICCO_SECTION_SLIDE_SWITCH));
    addSettingByKey(out, allSettings, "slideSwitchAction");
  }
  if (hasSettingByName(upstream, StrId::STR_FRONTLIGHT_SCHEDULE)) {
    out.push_back(SettingInfo::SectionHeader(StrId::STR_PICCO_SECTION_SCHEDULE));
    copyByName(out, upstream, StrId::STR_FRONTLIGHT_SCHEDULE);
    copyByName(out, upstream, StrId::STR_START);
    copyByName(out, upstream, StrId::STR_END);
  }
  if (hasSettingByName(upstream, StrId::STR_RESTORE_LIGHT_ON_WAKE)) {
    out.push_back(SettingInfo::SectionHeader(StrId::STR_PICCO_SECTION_WAKE));
    copyByName(out, upstream, StrId::STR_RESTORE_LIGHT_ON_WAKE);
  }
}

// T_Controls(2), T_ControlsTilt: Buttons, Touch, Gestures, Tilt, Footnotes. Power settings and
// taps come up from their sub-pages; the touchscreen toggle comes over from Text & fonts.
void SettingsActivity::reflowPiccoControls(const std::vector<SettingInfo>& allSettings) {
  const std::vector<SettingInfo> upstream = std::move(controlsSettings);
  controlsSettings.clear();
  controlsSettings.reserve(upstream.size() + controlsPowerSettings.size() + controlsTapsGesturesSettings.size() + 6);
  std::vector<SettingInfo>& out = controlsSettings;
  const bool slideSwitchInLight = Frontlight.present();

  out.push_back(SettingInfo::SectionHeader(StrId::STR_PICCO_SECTION_BUTTONS));
  for (const auto& s : controlsPowerSettings) {
    if (s.nameId != StrId::STR_PWR_BTN_FOOTNOTE_BACK) out.push_back(s);
  }
  for (const auto& s : upstream) {
    const bool button =
        s.action == SettingAction::ControlsHomeButton || s.action == SettingAction::ControlsFrontButtons ||
        s.action == SettingAction::ControlsSideButtons || s.action == SettingAction::QuickActions ||
        settingKeyIs(s, "menuNavigation") || (!slideSwitchInLight && settingKeyIs(s, "slideSwitchAction"));
    if (button) out.push_back(s);
  }

  const size_t touchAt = out.size();
  out.push_back(SettingInfo::SectionHeader(StrId::STR_PICCO_SECTION_TOUCH));
  if (copyByName(out, readerSettings, StrId::STR_DISABLE_TOUCHSCREEN)) {
    eraseByName(readerSettings, StrId::STR_DISABLE_TOUCHSCREEN);
  }
  for (const auto& s : controlsTapsGesturesSettings) {
    if (s.type != SettingType::SUBMENU && s.nameId != StrId::STR_TWO_FINGER_ROTATION) out.push_back(s);
  }
  if (out.size() == touchAt + 1) out.pop_back();

  const size_t gesturesAt = out.size();
  out.push_back(SettingInfo::SectionHeader(StrId::STR_PICCO_SECTION_GESTURES));
  for (const auto& s : controlsTapsGesturesSettings) {
    if (s.type == SettingType::SUBMENU || s.nameId == StrId::STR_TWO_FINGER_ROTATION) out.push_back(s);
  }
  if (out.size() == gesturesAt + 1) out.pop_back();

  const size_t tiltAt = out.size();
  out.push_back(SettingInfo::SectionHeader(StrId::STR_PICCO_SECTION_TILT));
  copyByName(out, upstream, StrId::STR_TILT_PAGE_TURN);
  copyByName(out, upstream, StrId::STR_TILT_PAGE_TURN_DIRECTION);
  copyByName(out, upstream, StrId::STR_TILT_SENSITIVITY);
  copyByName(out, upstream, StrId::STR_TILT_NEXT_PAGE_ONLY);
  if (out.size() == tiltAt + 1) out.pop_back();

  if (hasSettingByName(controlsPowerSettings, StrId::STR_PWR_BTN_FOOTNOTE_BACK)) {
    out.push_back(SettingInfo::SectionHeader(StrId::STR_PICCO_SECTION_FOOTNOTES));
    copyByName(out, controlsPowerSettings, StrId::STR_PWR_BTN_FOOTNOTE_BACK);
  }
  (void)allSettings;
}

// T_SettingsSystem(2): Language & time, Device, Files, then the remaining System rows under More.
void SettingsActivity::reflowPiccoSystem() {
  const std::vector<SettingInfo> upstream = std::move(systemSettings);
  systemSettings.clear();
  systemSettings.reserve(systemDeviceSettings.size() + systemFilesCacheSettings.size() + upstream.size() + 4);
  std::vector<SettingInfo>& out = systemSettings;
  const auto isDeviceRow = [](const SettingInfo& s) {
    return s.nameId == StrId::STR_DEVICE_NAME || s.nameId == StrId::STR_CUSTOM_BOOTSCREEN ||
           s.action == SettingAction::KeyboardLayouts || s.action == SettingAction::FilenameFallbackFont;
  };

  out.push_back(SettingInfo::SectionHeader(StrId::STR_PICCO_SECTION_LANGUAGE_TIME));
  for (const auto& s : systemDeviceSettings) {
    // Sleep after lives on Display & sleep.
    if (!isDeviceRow(s) && s.nameId != StrId::STR_TIME_TO_SLEEP) out.push_back(s);
  }
  out.push_back(SettingInfo::SectionHeader(StrId::STR_SYSTEM_DEVICE));
  for (const auto& s : systemDeviceSettings) {
    if (isDeviceRow(s) && s.action != SettingAction::KeyboardLayouts && s.action != SettingAction::FilenameFallbackFont)
      out.push_back(s);
  }
  for (const auto& s : upstream) {
    if (s.action == SettingAction::Plugins) out.push_back(s);
  }
  for (const auto& s : systemDeviceSettings) {
    if (s.action == SettingAction::KeyboardLayouts || s.action == SettingAction::FilenameFallbackFont) out.push_back(s);
  }
  out.push_back(SettingInfo::SectionHeader(StrId::STR_PICCO_SECTION_FILES));
  out.insert(out.end(), systemFilesCacheSettings.begin(), systemFilesCacheSettings.end());
  out.push_back(SettingInfo::SectionHeader(StrId::STR_PICCO_SECTION_MORE));
  for (const auto& s : upstream) {
    if (s.action != SettingAction::SystemDevice && s.action != SettingAction::SystemFilesCache &&
        s.action != SettingAction::Plugins) {
      out.push_back(s);
    }
  }
}

void SettingsActivity::selectFirstPiccoRow() {
  selectedSettingIndex = 1;
  for (int i = 0; i < settingsCount; ++i) {
    if ((*currentSettings)[i].type != SettingType::SECTION_HEADER) {
      selectedSettingIndex = i + 1;
      break;
    }
  }
  piccoFollowSelection = true;
}

bool SettingsActivity::openPiccoRootRow(const SettingInfo& setting) {
  if (!usesPiccoSettings() || !piccoRoot) return false;
  const int category = categoryFor(setting.action);
  if (category >= 0) {
    enterCategory(category);
    selectFirstPiccoRow();
    return true;
  }
  if (setting.action == SettingAction::DisplayFrontlight) {
    enterCategory(0);
    openSubmenu(SettingAction::DisplayFrontlight);
    piccoDirectSubmenu = SettingAction::DisplayFrontlight;
    piccoDirectTitle = setting.nameId;
    selectFirstPiccoRow();
    return true;
  }
  return false;
}

const char* SettingsActivity::piccoTitle() const {
  if (piccoRoot) return tr(STR_SETTINGS_TITLE);
  if (activeSubmenu != SettingAction::None && activeSubmenu == piccoDirectSubmenu) return I18N.get(piccoDirectTitle);
  const StrId submenu = activeSubmenuTitleId();
  return I18N.get(submenu != StrId::STR_NONE_OPT ? submenu : categoryTitle(selectedCategoryIndex));
}

void SettingsActivity::piccoBack() {
  if (piccoPicker.open) {
    closePiccoPicker();
    return;
  }
  if (piccoRoot) {
    closeRootSettings();
    return;
  }
  if (activeSubmenu != SettingAction::None && activeSubmenu != piccoDirectSubmenu) {
    closeSubmenu();
    piccoFollowSelection = true;
    requestUpdate();
    return;
  }
  const SettingAction opened =
      piccoDirectSubmenu != SettingAction::None ? piccoDirectSubmenu : rootActionFor(selectedCategoryIndex);
  activeSubmenu = SettingAction::None;
  parentSubmenu = SettingAction::None;
  piccoDirectSubmenu = SettingAction::None;
  piccoRoot = true;
  setCurrentSettingsForCategory();
  selectFirstPiccoRow();
  for (int i = 0; i < settingsCount; ++i) {
    if ((*currentSettings)[i].type == SettingType::SUBMENU && (*currentSettings)[i].action == opened) {
      selectedSettingIndex = i + 1;
      break;
    }
  }
  requestUpdate();
}

void SettingsActivity::stepPiccoSetting(const int index, const int direction) {
  if (index < 0 || index >= settingsCount) return;
  const SettingInfo& setting = (*currentSettings)[index];
  if (setting.nameId == StrId::STR_TIME_TO_SLEEP) {
    SETTINGS.sleepTimeoutMinutes = static_cast<uint8_t>(PiccoValueSteps::step(
        PiccoValueSteps::kSleepMinutes, PiccoValueSteps::kSleepMinutesCount, SETTINGS.sleepTimeoutMinutes, direction));
  } else if (setting.type == SettingType::ENUM && setting.valuePtr != nullptr) {
    const int count = static_cast<int>(settingEnumOptionCount(setting));
    const int next = PiccoValueSteps::stepIndex(enumIndexOf(setting, SETTINGS.*(setting.valuePtr)), count, direction);
    SETTINGS.*(setting.valuePtr) = enumRawAt(setting, next);
  } else {
    return;
  }
  SETTINGS.saveToFile();
  selectedSettingIndex = index + 1;
  piccoFollowSelection = true;
  requestUpdate();
}

bool SettingsActivity::openPiccoSleepPicker() {
  if (!usesPiccoSettings()) return false;
  std::vector<std::string> options;
  options.reserve(PiccoValueSteps::kSleepMinutesCount);
  char label[32];
  for (const uint16_t minutes : PiccoValueSteps::kSleepMinutes) {
    formatSleepMinutes(minutes, label, sizeof(label));
    options.emplace_back(label);
  }
  const int current = PiccoValueSteps::indexFor(PiccoValueSteps::kSleepMinutes, PiccoValueSteps::kSleepMinutesCount,
                                                SETTINGS.sleepTimeoutMinutes);
  openPiccoPicker(
      StrId::STR_TIME_TO_SLEEP, std::move(options), current,
      [this](const int choice) {
        if (choice < 0 || choice >= PiccoValueSteps::kSleepMinutesCount) return;
        SETTINGS.sleepTimeoutMinutes = static_cast<uint8_t>(PiccoValueSteps::kSleepMinutes[choice]);
        SETTINGS.saveToFile();
      },
      nullptr);
  return true;
}

bool SettingsActivity::openPiccoLanguagePicker() {
  if (!usesPiccoSettings()) return false;
  RenderLock lock(*this);
  languageCatalog.reset();
  // Same bounded catalog as the popup path; the picker copies the names it shows.
  languageCatalog.init(MemoryPool::None);
  const auto status = languageCatalog ? I18N.discover(*languageCatalog.get()) : language_cache::Result::Memory;
  const bool cached = std::strcmp(I18N.getCode(), "EN") != 0;
  std::vector<std::string> options;
  int current = cached ? 1 : 0;
  bool duplicates = false;
  if (status == language_cache::Result::Ok) {
    options.reserve(languageCatalog->count);
    for (size_t i = 0; i < languageCatalog->count; ++i) {
      if (std::strcmp(languageCatalog->code(i), I18N.getCode()) == 0) current = static_cast<int>(i);
      duplicates = duplicates || languageCatalog->disabled(i);
      options.emplace_back(languageCatalog->name(i));
    }
  } else {
    LOG_ERR("LANG", "Language picker: %s", language_cache::resultName(status));
    languageCatalog.reset();
    options.emplace_back("English");
    if (cached) options.emplace_back(I18N.getName());
  }
  const char* note = status == language_cache::Result::Ok
                         ? (duplicates ? tr(STR_LANGUAGE_DUPLICATE) : tr(STR_LANGUAGE_APPLY_HINT))
                     : status == language_cache::Result::Memory   ? tr(STR_MEMORY_ERROR)
                     : status == language_cache::Result::TooLarge ? tr(STR_LANGUAGE_CATALOG_LIMIT)
                                                                  : tr(STR_LANGUAGE_IO_ERROR);
  openPiccoPicker(
      StrId::STR_LANGUAGE, std::move(options), current,
      [this](const int index) {
        if (index < 0) return;
        if (languageCatalog) {
          if (static_cast<size_t>(index) >= languageCatalog->count || languageCatalog->disabled(index)) return;
          languageCatalog->select(index, pendingLanguage);
        } else {
          std::strcpy(pendingLanguage.code, index ? I18N.getCode() : "EN");
          pendingLanguage.path[0] = '\0';
          pendingLanguage.generation = index ? I18N.getGeneration() : 0;
        }
        // Installation starts on the next loop, after this callback returns.
      },
      note);
  return true;
}

bool SettingsActivity::openPiccoPluginsPicker() {
  if (!usesPiccoSettings()) return false;
  std::vector<std::string> options{tr(STR_ENABLED), tr(STR_DISABLED)};
  openPiccoPicker(
      StrId::STR_PLUGINS, std::move(options), SETTINGS.pluginsEnabled ? 0 : 1,
      [this](const int index) {
        SETTINGS.pluginsEnabled = index == 0 ? 1 : 0;
        pluginevents::refreshSubscriptions();
        if (SETTINGS.pluginsEnabled) SETTINGS.pluginHubPromptHidden = 0;
        SETTINGS.saveToFile();
        rebuildSettingsLists();
      },
      nullptr);
  return true;
}

void SettingsActivity::openPiccoPicker(const StrId title, std::vector<std::string> options, const int current,
                                       std::function<void(int)> onSelect, const char* note) {
  piccoPicker.open = true;
  piccoPicker.title = title;
  piccoPicker.options = std::move(options);
  piccoPicker.current = current;
  piccoPicker.onSelect = std::move(onSelect);
  piccoPicker.note = note;
  piccoPickerSelected = current;
  piccoFollowSelection = true;
  requestUpdate();
}

void SettingsActivity::closePiccoPicker() {
  piccoPicker = PiccoPicker{};
  piccoFollowSelection = true;
  requestUpdate();
}

void SettingsActivity::piccoListInput(const int count, int& selected, const std::function<bool(int)>& isRow,
                                      const std::function<void()>& activate, const std::function<void()>& back) {
  if (TouchHeaderBackButton::wasTapped(mappedInput, renderer, piccoTitle()) ||
      mappedInput.wasPressed(MappedInputManager::Button::Back)) {
    back();
    return;
  }

  int tapped = -1;
  if (mappedInput.wasItemTapped(tapped)) {
    if (!isRow(tapped)) return;
    selected = tapped;
    piccoFocus = false;
    activate();
    return;
  }

  const auto swipe = mappedInput.wasSwipe();
  if (dismissOnUpSwipe && swipe == MappedInputManager::SwipeDir::Up && mappedInput.wasBottomEdgeUpSwipe()) {
    SETTINGS.saveToFile();
    finishToParent();
    return;
  }
  const int pages = std::clamp(piccoPageCount, 1, kPiccoMaxPages);
  const auto showPage = [&](const int page) {
    if (page < 0 || page >= pages || page == piccoPage) return;
    piccoPage = page;
    piccoFollowSelection = false;
    if (piccoFocus) {
      const int end = page + 1 < pages ? piccoPageStarts[page + 1] : count;
      for (int i = piccoPageStarts[page]; i < end; ++i) {
        if (isRow(i)) {
          selected = i;
          break;
        }
      }
    }
    requestUpdate();
  };
  if (swipe == MappedInputManager::SwipeDir::Up) {
    showPage(piccoPage + 1);
    return;
  }
  if (swipe == MappedInputManager::SwipeDir::Down) {
    showPage(piccoPage - 1);
    return;
  }

  // The rocker moves row by row; the render follows the selection onto its page.
  const auto step = [&](const int direction) {
    if (!piccoFocus) {
      piccoFocus = true;
      int index = piccoPageStarts[std::clamp(piccoPage, 0, kPiccoMaxPages - 1)];
      while (index < count && !isRow(index)) ++index;
      if (index < count) selected = index;
    } else {
      int index = selected;
      for (int n = 0; n < count; ++n) {
        index += direction;
        if (index >= count) index = 0;
        if (index < 0) index = count - 1;
        if (isRow(index)) break;
      }
      if (isRow(index)) selected = index;
    }
    piccoFollowSelection = true;
    requestUpdate();
  };
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (!piccoFocus) {
      step(1);
    } else if (isRow(selected)) {
      activate();
    }
    return;
  }
  buttonNavigator.onNext([&] { step(1); });
  buttonNavigator.onPrevious([&] { step(-1); });
}

void SettingsActivity::piccoLoop() {
  if (piccoPicker.open) {
    const int count = static_cast<int>(piccoPicker.options.size());
    piccoListInput(
        count, piccoPickerSelected, [count](const int i) { return i >= 0 && i < count; },
        [this] {
          // Keep the callback alive past closePiccoPicker(), which resets the picker.
          auto onSelect = std::move(piccoPicker.onSelect);
          const int choice = piccoPickerSelected;
          closePiccoPicker();
          if (onSelect) onSelect(choice);
        },
        [this] { closePiccoPicker(); });
    return;
  }
  int tappedId = -1;
  if (mappedInput.wasItemTapped(tappedId) && tappedId >= kStepperIdBase) {
    stepPiccoSetting((tappedId - kStepperIdBase) / 2, (tappedId - kStepperIdBase) % 2 ? 1 : -1);
    return;
  }
  int selected = selectedSettingIndex - 1;
  bool handled = false;  // activation and back pick their own selection
  piccoListInput(
      settingsCount, selected,
      [this](const int index) {
        return index >= 0 && index < settingsCount && (*currentSettings)[index].type != SettingType::SECTION_HEADER;
      },
      [this, &selected, &handled] {
        handled = true;
        selectedSettingIndex = selected + 1;
        toggleCurrentSetting();
        piccoFollowSelection = true;
        requestUpdate();
      },
      [this, &handled] {
        handled = true;
        piccoBack();
      });
  if (!handled && selected >= 0 && selected < settingsCount) selectedSettingIndex = selected + 1;
}

void SettingsActivity::drawPiccoList(const char* title, const Row* rows, const int count, const int selected,
                                     const char* note, const bool versionFooter) {
  renderer.clearScreen();
  if (mappedInput.hasTouchHardware()) {
    TouchHeaderBackButton::drawCompact(renderer, title);
  } else {
    CompactHeader::drawTitle(renderer, title);
  }
  const Rect header = TouchHeaderBackButton::compactHeaderRect(renderer);
  int top = header.y + header.height;
  int bottom = PiccoPageIndicator::listBottom(renderer);
  Rect band = PiccoPageIndicator::band(renderer);
  const int width = renderer.getScreenWidth() - 2 * PiccoTokens::kGutter;

  if (note != nullptr && note[0] != '\0') {
    // A help note sits between the header and the rows. Before a section (T_SettingsLight) it
    // takes the first section's 20 px and the section keeps its regular 30 above.
    constexpr int kNoteLines = 3;
    const bool sectionNext = count > 0 && rows[0].section;
    const auto lines = renderer.wrappedText(SMALL_FONT_ID, note, width, kNoteLines);
    int y = top + (sectionNext ? PiccoTokens::kSectionFirstAbove : PiccoTokens::kGroupGap);
    for (const auto& line : lines) {
      renderer.drawText(SMALL_FONT_ID, PiccoTokens::kGutter, y, line.c_str());
      y += renderer.getLineHeight(SMALL_FONT_ID);
    }
    top = sectionNext ? y + PiccoTokens::kSectionAbove - PiccoTokens::kSectionFirstAbove : y;
  }
  if (versionFooter) {
    // The version keeps the indicator's band; the indicator and the list move up.
    const std::string version = AppVersion::versionLabel();
    const std::string line = renderer.truncatedText(SMALL_FONT_ID, version.c_str(), width);
    const int footerY = band.y + (band.height - renderer.getLineHeight(SMALL_FONT_ID)) / 2;
    renderer.drawText(SMALL_FONT_ID,
                      (renderer.getScreenWidth() - renderer.getTextWidth(SMALL_FONT_ID, line.c_str())) / 2, footerY,
                      line.c_str());
    band.y -= band.height;
    bottom = std::min(bottom, band.y - PiccoTokens::kGroupGap);
  }

  std::vector<PiccoPagedList::Entry> entries(count);
  std::vector<PiccoPagedList::Slot> slots(count);
  for (int i = 0; i < count; ++i) {
    entries[i].section = rows[i].section;
    entries[i].height = PiccoSettingsRows::rowHeight(rows[i]);
  }
  const int pages =
      PiccoPagedList::paginate(entries.data(), count, top, bottom, PiccoSettingsRows::sectionTextHeight(renderer),
                               slots.data(), piccoPageStarts, kPiccoMaxPages, /*continuedLabels=*/true);
  piccoPageCount = pages;
  if ((piccoFocus || piccoFollowSelection) && selected >= 0 && selected < count) {
    piccoPage = PiccoPagedList::pageOf(piccoPageStarts, pages, selected);
  }
  piccoPage = std::clamp(piccoPage, 0, pages - 1);
  const int first = count > 0 ? piccoPageStarts[piccoPage] : 0;
  const int end = PiccoPagedList::pageEnd(piccoPageStarts, pages, piccoPage, count);
  if (PiccoPagedList::opensContinued(entries.data(), count, first)) {
    const int owner = PiccoPagedList::owningSection(entries.data(), first);
    PiccoSettingsRows::drawContinued(renderer, rows[owner].label, tr(STR_PICCO_CONTINUED_FORMAT),
                                     PiccoPagedList::continuedLabelY(top));
  }
  PiccoSettingsRows::drawPage(renderer, rows, slots.data(), first, end, piccoFocus ? selected : -1);
  for (int i = first; i < end; ++i) {
    if (rows[i].section) continue;
    // Later registrations win the hit test: the stepper buttons sit over their row.
    TouchRegistry::getInstance().add(PiccoSettingsRows::rowRect(renderer, slots[i]), i, TouchRegistry::Item);
    if (rows[i].trailing == Trailing::Stepper) {
      const auto stepper = PiccoSettingsRows::stepperRects(renderer, slots[i]);
      const auto& m = stepper.minus;
      const auto& p = stepper.plus;
      TouchRegistry::getInstance().add(Rect{m.x, m.y, m.width, m.height}, stepperId(i, false), TouchRegistry::Item);
      TouchRegistry::getInstance().add(Rect{p.x, p.y, p.width, p.height}, stepperId(i, true), TouchRegistry::Item);
    }
  }
  PiccoPageIndicator::draw(renderer, piccoPage + 1, pages, &band);
  renderer.displayBuffer();
}

void SettingsActivity::renderPicco() {
  if (piccoPicker.open) {
    const int count = static_cast<int>(piccoPicker.options.size());
    std::vector<Row> rows(count);
    for (int i = 0; i < count; ++i) {
      rows[i].label = piccoPicker.options[i].c_str();
      rows[i].trailing = Trailing::Check;
      rows[i].on = i == piccoPicker.current;
      rows[i].bold = rows[i].on;
    }
    drawPiccoList(I18N.get(piccoPicker.title), rows.data(), count, piccoPickerSelected, piccoPicker.note, false);
    return;
  }

  const auto& settings = *currentSettings;
  const int count = static_cast<int>(settings.size());
  std::vector<std::string> values(count);
  std::vector<Row> rows(count);
  const std::string leftUpLabel = sideButtonGroupLabel(true);
  const std::string rightDownLabel = sideButtonGroupLabel(false);
  for (int i = 0; i < count; ++i) {
    const SettingInfo& setting = settings[i];
    Row& row = rows[i];
    if (setting.type == SettingType::SECTION_HEADER) {
      row.section = true;
      row.label = activeSubmenu == SettingAction::ControlsSideButtons
                      ? (setting.nameId == StrId::STR_DIR_LEFT ? leftUpLabel.c_str() : rightDownLabel.c_str())
                      : I18N.get(setting.nameId);
      continue;
    }
    row.label = I18N.get(setting.nameId);
    // The flat Controls page names the power rows in full (T_Controls).
    if (setting.valuePtr == &CrossPointSettings::shortPwrBtn) row.label = tr(STR_PICCO_POWER_BUTTON);
    if (setting.valuePtr == &CrossPointSettings::longPwrBtn) row.label = tr(STR_PICCO_POWER_LONG_PRESS);
    row.hint = piccoHint(setting);
    if (setting.type == SettingType::TOGGLE && setting.valuePtr != nullptr) {
      row.trailing = Trailing::Switch;
      row.on = SETTINGS.*(setting.valuePtr) != 0;
      if (setting.valuePtr == &CrossPointSettings::disableReaderTouchscreen) {
        // T_Controls shows the positive "Touchscreen" switch; the stored value is "disable".
        row.label = tr(STR_PICCO_TOUCHSCREEN);
        row.on = !row.on;
      }
      continue;
    }
    values[i] = settingValueText(setting);
    if (values[i] == ">") values[i].clear();
    if (setting.action == SettingAction::DisplaySleepScreen && !displaySleepSettings.empty()) {
      values[i] = settingValueText(displaySleepSettings.front());
    }
    if (isPiccoStepper(setting)) {
      if (setting.nameId == StrId::STR_TIME_TO_SLEEP) {
        char sleep[32];
        formatSleepMinutes(SETTINGS.sleepTimeoutMinutes, sleep, sizeof(sleep));
        values[i] = sleep;
      }
      row.value = values[i].c_str();
      row.trailing = Trailing::Stepper;
      continue;
    }
    if (setting.action == SettingAction::PiccoReading) {
      // T_Settings: the reading font and its size.
      std::string family;
      std::string size;
      for (const auto& font : readerFontSettings) {
        if (font.nameId == StrId::STR_FONT_FAMILY) family = settingValueText(font);
        if (font.nameId == StrId::STR_FONT_SIZE) size = settingValueText(font);
      }
      values[i] = family.empty() || size.empty() ? family + size : family + " \xC2\xB7 " + size;
    }
    if (setting.action == SettingAction::PiccoDisplay) {
      char sleep[32];
      formatSleepMinutes(SETTINGS.sleepTimeoutMinutes, sleep, sizeof(sleep));
      values[i] = sleep;
    }
    row.value = values[i].c_str();
    const bool picker =
        opensSomething(setting) || currentSettingUsesOptionMenu(setting) || setting.nameId == StrId::STR_FONT_FAMILY;
    row.trailing = picker ? Trailing::ValueChevron : Trailing::Value;
  }
  // Rows per page vary with sections; the shared row-paging helpers see a single page.
  visibleRows = std::max(1, count);
  const bool versionFooter = !piccoRoot && selectedCategoryIndex == 3 && activeSubmenu == SettingAction::None;
  const char* note = activeSubmenu == SettingAction::DisplayFrontlight && slideSwitchOwnsLight()
                         ? tr(STR_PICCO_NOTE_SLIDE_SWITCH_LIGHT)
                         : nullptr;
  drawPiccoList(piccoTitle(), rows.data(), count, selectedSettingIndex - 1, note, versionFooter);
}

#endif
