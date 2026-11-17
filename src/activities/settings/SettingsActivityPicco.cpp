// Picco Settings (T_Settings, T_Settings2): grouped root and hand-drawn paged rows. The
// category pages reuse SettingsActivity's lists, so every option stays where it was below
// the root; only the four tabs are replaced.
#include "SettingsActivity.h"

#if CROSSINK_APP_PICCO_UI

#include <CrossInkHalFrontlight.h>
#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>
#include <cstdio>
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
#include "fontIds.h"

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
    // A picker's help note sits between the header and its rows.
    constexpr int kNoteLines = 3;
    const auto lines = renderer.wrappedText(SMALL_FONT_ID, note, width, kNoteLines);
    int y = top + PiccoTokens::kGroupGap;
    for (const auto& line : lines) {
      renderer.drawText(SMALL_FONT_ID, PiccoTokens::kGutter, y, line.c_str());
      y += renderer.getLineHeight(SMALL_FONT_ID);
    }
    top = y;
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
                               slots.data(), piccoPageStarts, kPiccoMaxPages);
  piccoPageCount = pages;
  if ((piccoFocus || piccoFollowSelection) && selected >= 0 && selected < count) {
    piccoPage = PiccoPagedList::pageOf(piccoPageStarts, pages, selected);
  }
  piccoPage = std::clamp(piccoPage, 0, pages - 1);
  const int first = count > 0 ? piccoPageStarts[piccoPage] : 0;
  const int end = PiccoPagedList::pageEnd(piccoPageStarts, pages, piccoPage, count);
  PiccoSettingsRows::drawPage(renderer, rows, slots.data(), first, end, piccoFocus ? selected : -1);
  for (int i = first; i < end; ++i) {
    if (!rows[i].section) {
      TouchRegistry::getInstance().add(PiccoSettingsRows::rowRect(renderer, slots[i]), i, TouchRegistry::Item);
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
    if (setting.type == SettingType::TOGGLE && setting.valuePtr != nullptr) {
      row.trailing = Trailing::Switch;
      row.on = SETTINGS.*(setting.valuePtr) != 0;
      continue;
    }
    values[i] = settingValueText(setting);
    if (values[i] == ">") values[i].clear();
    if (setting.action == SettingAction::PiccoDisplay) {
      char sleep[32];
      if (SETTINGS.sleepTimeoutMinutes >= CrossPointSettings::SLEEP_TIMEOUT_NEVER_MINUTES) {
        std::snprintf(sleep, sizeof(sleep), "%s", tr(STR_NEVER));
      } else {
        std::snprintf(sleep, sizeof(sleep), tr(STR_SLEEP_TIMER_VALUE_FORMAT),
                      static_cast<unsigned>(SETTINGS.sleepTimeoutMinutes));
      }
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
  drawPiccoList(piccoTitle(), rows.data(), count, selectedSettingIndex - 1, nullptr, versionFooter);
}

#endif
