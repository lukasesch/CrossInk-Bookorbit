#include "ReaderSettingsController.h"

#include <Epub.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <Logging.h>
#include <SdCardFontSystem.h>

#include <algorithm>
#include <cstring>
#include <iterator>

#include "CrossPointSettings.h"
#include "ReaderFontLoading.h"
#include "ReaderUtils.h"
#include "SettingsList.h"
#include "activities/RenderLock.h"
#include "components/UITheme.h"
#include "util/Dictionary.h"
#include "util/DictionaryRegistry.h"
#include "util/FontFamilyLabel.h"
#include "util/ImageDisplayMode.h"
#if CROSSINK_SCALABLE_FONTS
#include "BookFontSystem.h"
#include "activities/settings/TtfRenderOptionsActivity.h"
#endif

ReaderSettingsController::ReaderSettingsController(const ReaderSettingsDraft& current, const Callbacks& callbacks,
                                                   const char* dictionaryFontFamilyName,
                                                   const uint8_t dictionaryFontPointSize,
                                                   const bool hasDictionaryFontOverride)
    : draft(current),
      sourceSettings(current),
      lastGoodPreviewSettings(current),
      dictionaryFontPointSize(dictionaryFontPointSize),
      hasDictionaryFontOverride(hasDictionaryFontOverride),
      callbacks(callbacks) {
  if (dictionaryFontFamilyName) {
    std::strncpy(this->dictionaryFontFamilyName, dictionaryFontFamilyName, sizeof(this->dictionaryFontFamilyName) - 1);
  }
}

void ReaderSettingsController::discoverFonts(GfxRenderer& renderer) {
  RenderLock lock;
  GUI.drawPopup(renderer, tr(STR_LOADING), true);
  sdFontSystem.refreshIfDirty();
  const auto& families = sdFontSystem.registry().getFamilies();
  fontLabels.clear();
  fontSettingIndexes.clear();
  fontLabels.reserve(CrossPointSettings::BUILTIN_FONT_COUNT + families.size());
  fontSettingIndexes.reserve(CrossPointSettings::BUILTIN_FONT_COUNT + families.size());
  constexpr auto builtinRange = BUILTIN_FONT_POINT_SIZE_RANGE;
  fontLabels.push_back(fontFamilyLabel(tr(STR_LEXEND_DECA), builtinRange));
  fontLabels.push_back(fontFamilyLabel(tr(STR_BITTER), builtinRange));
  fontSettingIndexes.push_back(0);
  fontSettingIndexes.push_back(1);
  for (size_t i = 0; i < families.size(); ++i) {
    fontLabels.push_back(fontFamilyLabel(families[i].name, fontFamilyPointSizeRange(families[i])));
    fontSettingIndexes.push_back(static_cast<uint8_t>(CrossPointSettings::BUILTIN_FONT_COUNT + i));
  }
}

#if CROSSINK_SCALABLE_FONTS
void ReaderSettingsController::beginTtfRenderingEdit() {
  ttfRenderProfile = TTF_RENDER_PROFILES.profileFor(draft.sdFontFamilyName.data());
  initialTtfRenderProfile = ttfRenderProfile;
  ttfRenderingChanged = false;
}

bool ReaderSettingsController::saveTtfRenderingProfile() {
  const char* const family = draft.sdFontFamilyName.data();
  if (ttfRenderProfile == TTF_RENDER_PROFILES.profileFor(family)) return true;
  if (!TTF_RENDER_PROFILES.setProfile(family, ttfRenderProfile)) {
    LOG_ERR("ERDM", "Failed to save TTF rendering profile for %s", family);
    return false;
  }
  ttfRenderingChanged = ttfRenderProfile != initialTtfRenderProfile;
  return true;
}

void ReaderSettingsController::finishTtfRenderingEdit(GfxRenderer& renderer) {
  if (!ttfRenderingChanged) return;
  if (sdFontSystem.reloadActiveScalableFamily(renderer, draft.sdFontFamilyName.data())) {
    previewFontMetricsChanged = true;
    markSettingChanged(ReaderSettingsChangeMask::Preview | ReaderSettingsChangeMask::Relayout);
  }
  ttfRenderingChanged = false;
}

bool ReaderSettingsController::showTtfRenderingOptions(const RowId row) {
  using TtfRow = TtfRenderOptionsActivity::Row;
  TtfRow optionRow;
  switch (row) {
    case RowId::TtfHinting:
      optionRow = TtfRow::Hinting;
      break;
    case RowId::TtfRaster:
      optionRow = TtfRow::Raster;
      break;
    case RowId::TtfInterpreter:
      optionRow = TtfRow::Interpreter;
      break;
    case RowId::TtfWeight:
      optionRow = TtfRow::Weight;
      break;
    case RowId::TtfSlant:
      optionRow = TtfRow::Slant;
      break;
    default:
      return false;
  }
  const auto* options = TtfRenderOptionsActivity::optionLabels(optionRow);
  if (options == nullptr) return false;
  std::vector<uint8_t> values;
  values.reserve(options->size());
  for (size_t i = 0; i < options->size(); ++i) values.push_back(static_cast<uint8_t>(i));
  return openEnumOptions(row, TtfRenderOptionsActivity::titleId(optionRow), *options, std::move(values),
                         TtfRenderOptionsActivity::selectedOption(ttfRenderProfile, optionRow));
}

void ReaderSettingsController::toggleTtfStemDarkening() {
  ttfRenderProfile.stemDarkening = !ttfRenderProfile.stemDarkening;
  saveTtfRenderingProfile();
}

bool ReaderSettingsController::resetTtfRenderingProfile() {
  const TtfRenderProfile previous = ttfRenderProfile;
  ttfRenderProfile = {};
  const bool saved = saveTtfRenderingProfile();
  if (!saved) ttfRenderProfile = previous;
  return saved;
}
#endif

void ReaderSettingsController::discoverDictionaries(const Epub* epub) {
  dictionaryRegistry.discover();
  dictionaryLabels.clear();
  dictionaryPaths.clear();
  dictionaryLabels.reserve(dictionaryRegistry.getEntries().size() + 1);
  dictionaryPaths.reserve(dictionaryRegistry.getEntries().size() + 1);
  dictionaryLabels.emplace_back(tr(STR_DICT_USE_GLOBAL));
  dictionaryPaths.emplace_back();
  for (const auto& entry : dictionaryRegistry.getEntries()) {
    dictionaryLabels.push_back(entry.name);
    dictionaryPaths.push_back(entry.basePath);
  }
  bookDictionaryPath = epub ? Dictionary::readDictPath(epub->getCachePath().c_str()) : std::string{};
}

bool ReaderSettingsController::selectBookDictionary(const Epub* epub, const int index) {
  if (index < 0 || index >= static_cast<int>(dictionaryPaths.size())) return false;
  if (!saveBookDictionary(epub, dictionaryPaths[static_cast<size_t>(index)])) return false;
  bookDictionaryPath = dictionaryPaths[static_cast<size_t>(index)];
  return true;
}

void ReaderSettingsController::releaseCatalogs() {
  dictionaryRegistry.clear();
  // The reader remains active beneath the menu. Keep the small catalog for
  // its resident scalable family so reopening Font Size avoids an SD rescan.
#if CROSSINK_SCALABLE_FONTS
  if (!sdFontSystem.hasResidentScalableFamily(SETTINGS.sdFontFamilyName))
#endif
    sdFontSystem.releaseRegistry();
}

ReaderSettingsCommit ReaderSettingsController::commitSettings(const bool requirePreviewedSdFont) {
  // A queued preview may not have rendered before Back or an external exit.
  // Save only an SD font whose preview already loaded and prewarmed.
  const ReaderSettingsCommit commit =
      prepareReaderSettingsCommit(changes.settingsChanged, requirePreviewedSdFont, draft, lastGoodPreviewSettings);
  if (commit == ReaderSettingsCommit::Unchanged) return commit;
  if (commit == ReaderSettingsCommit::SaveWithPreviousFont) {
    LOG_ERR("ERDM", "Selected SD font was not previewed; retaining previous reader font");
  }
  applySettings(draft);
  if (callbacks.saveReaderSettings) {
    callbacks.saveReaderSettings(callbacks.saveReaderSettingsContext);
  } else if (!SETTINGS.saveToFile()) {
    LOG_ERR("ERDM", "Failed to persist touch reader settings");
  }
  changes.settingsChanged = false;
  return commit;
}

ReaderSettingsDraft ReaderSettingsController::captureSettings() {
  ReaderSettingsDraft value;
  value.fontFamily = SETTINGS.fontFamily;
  value.readerFontPointSize = SETTINGS.readerFontPointSize;
  std::strncpy(value.sdFontFamilyName.data(), SETTINGS.sdFontFamilyName, value.sdFontFamilyName.size() - 1);
  value.lineHeightPercent = SETTINGS.lineHeightPercent;
  value.wordSpacing = SETTINGS.wordSpacing;
  value.characterSpacing = SETTINGS.characterSpacing;
  value.screenMarginVertical = SETTINGS.screenMarginVertical;
  value.screenMarginHorizontal = SETTINGS.screenMarginHorizontal;
  value.orientation = SETTINGS.orientation;
  value.paragraphAlignment = SETTINGS.paragraphAlignment;
  value.textAntiAliasing = SETTINGS.textAntiAliasing;
  value.focusReadingEnabled = SETTINGS.focusReadingEnabled;
  value.guideReadingEnabled = SETTINGS.guideReadingEnabled;
  value.hyphenationEnabled = SETTINGS.hyphenationEnabled;
  value.publisherPageNumbers = SETTINGS.publisherPageNumbers;
  value.extraParagraphSpacing = SETTINGS.extraParagraphSpacing;
  value.forceParagraphIndents = SETTINGS.forceParagraphIndents;
  value.embeddedStyle = SETTINGS.embeddedStyle;
  value.publisherFonts = SETTINGS.publisherFonts;
  value.imageRendering = SETTINGS.imageRendering;
  value.imageGrayscale = SETTINGS.imageGrayscale;
  value.epubRenderMode = SETTINGS.epubRenderMode;
  value.indexingMethod = SETTINGS.indexingMethod;
  return value;
}

void ReaderSettingsController::applySettings(const ReaderSettingsDraft& value) {
  SETTINGS.fontFamily = value.fontFamily;
  SETTINGS.readerFontPointSize = value.readerFontPointSize;
  std::strncpy(SETTINGS.sdFontFamilyName, value.sdFontFamilyName.data(), sizeof(SETTINGS.sdFontFamilyName) - 1);
  SETTINGS.sdFontFamilyName[sizeof(SETTINGS.sdFontFamilyName) - 1] = '\0';
  SETTINGS.lineHeightPercent = value.lineHeightPercent;
  SETTINGS.wordSpacing = value.wordSpacing;
  SETTINGS.characterSpacing = value.characterSpacing;
  SETTINGS.screenMarginVertical = value.screenMarginVertical;
  SETTINGS.screenMarginHorizontal = value.screenMarginHorizontal;
  SETTINGS.orientation = value.orientation;
  SETTINGS.paragraphAlignment = value.paragraphAlignment;
  SETTINGS.textAntiAliasing = value.textAntiAliasing;
  SETTINGS.focusReadingEnabled = value.focusReadingEnabled;
  SETTINGS.guideReadingEnabled = value.guideReadingEnabled;
  SETTINGS.hyphenationEnabled = value.hyphenationEnabled;
  SETTINGS.publisherPageNumbers = value.publisherPageNumbers;
  SETTINGS.extraParagraphSpacing = value.extraParagraphSpacing;
  SETTINGS.forceParagraphIndents = value.forceParagraphIndents;
  SETTINGS.embeddedStyle = value.embeddedStyle;
  SETTINGS.publisherFonts = value.publisherFonts;
  SETTINGS.imageRendering = value.imageRendering;
  SETTINGS.imageGrayscale = value.imageGrayscale;
  SETTINGS.epubRenderMode = value.epubRenderMode;
  SETTINGS.indexingMethod = value.indexingMethod;
}

void ReaderSettingsController::markSettingChanged(const ReaderSettingsChangeMask mask) {
  changes.mark(mask);
  previewDirty = previewDirty || hasReaderSettingsChange(mask, ReaderSettingsChangeMask::Preview);
}

void ReaderSettingsController::beginGlobalSettingsEdit() const {
  if (callbacks.beginGlobalSettingsEdit) callbacks.beginGlobalSettingsEdit(callbacks.beginGlobalSettingsEditContext);
}

void ReaderSettingsController::endGlobalSettingsEdit() const {
  if (callbacks.endGlobalSettingsEdit) callbacks.endGlobalSettingsEdit(callbacks.endGlobalSettingsEditContext);
}

void ReaderSettingsController::saveGlobalSettings() const {
  if (callbacks.saveGlobalSettings) callbacks.saveGlobalSettings(callbacks.saveGlobalSettingsContext);
}

int ReaderSettingsController::currentFontSelectionIndex(const int pendingFontIndex) const {
  int selectedFontIndex = pendingFontIndex;
  if (selectedFontIndex < 0) {
    if (draft.sdFontFamilyName[0] != '\0') {
      const auto& families = sdFontSystem.registry().getFamilies();
      const auto selected = std::find_if(families.begin(), families.end(), [this](const auto& family) {
        return family.name == draft.sdFontFamilyName.data();
      });
      if (selected != families.end()) {
        selectedFontIndex =
            static_cast<int>(CrossPointSettings::BUILTIN_FONT_COUNT + std::distance(families.begin(), selected));
      }
    } else {
      const auto selected = std::find(fontSettingIndexes.begin(), fontSettingIndexes.end(), draft.fontFamily);
      if (selected != fontSettingIndexes.end()) {
        selectedFontIndex = static_cast<int>(std::distance(fontSettingIndexes.begin(), selected));
      }
    }
  }
  return selectedFontIndex;
}

bool ReaderSettingsController::previewFontFamily(const int index) {
  if (index < 0 || index >= static_cast<int>(fontSettingIndexes.size())) return false;
  fontPreviewLoading = true;
  const uint8_t settingIndex = fontSettingIndexes[static_cast<size_t>(index)];
  if (settingIndex < CrossPointSettings::BUILTIN_FONT_COUNT) {
    draft.fontFamily = settingIndex;
    draft.sdFontFamilyName[0] = '\0';
  } else {
    const int familyIndex = settingIndex - CrossPointSettings::BUILTIN_FONT_COUNT;
    const auto& families = sdFontSystem.registry().getFamilies();
    if (familyIndex >= 0 && familyIndex < static_cast<int>(families.size())) {
      std::strncpy(draft.sdFontFamilyName.data(), families[static_cast<size_t>(familyIndex)].name.c_str(),
                   draft.sdFontFamilyName.size() - 1);
      draft.sdFontFamilyName.back() = '\0';
      // The render task loads the draft font when rebuilding its preview.
    }
  }
  markSettingChanged(ReaderSettingsChangeMask::Preview | ReaderSettingsChangeMask::Relayout);
  return true;
}

bool ReaderSettingsController::toggleSetting(const RowId row) {
  const ReaderSettingsChangeMask mask = toggleReaderSetting(draft, row);
  if (mask == ReaderSettingsChangeMask::None) return false;
  markSettingChanged(mask);
  return true;
}

bool ReaderSettingsController::showEnumOptions(GfxRenderer& renderer, const RowId row) {
  RenderLock lock;
  const bool fontSizeNeedsLoading = row == RowId::FontSize && draft.sdFontFamilyName[0] != '\0' &&
                                    ReaderUtils::shouldShowFontPreviewLoading(draft.sdFontFamilyName.data());
  if (row == RowId::DictionaryFontFamily || row == RowId::DictionaryFontSize || fontSizeNeedsLoading) {
    GUI.drawPopup(renderer, tr(STR_LOADING), true);
  }
  if (row == RowId::DictionaryFontFamily || row == RowId::DictionaryFontSize) sdFontSystem.refreshIfDirty();
  std::vector<std::string> labels;
  std::vector<uint8_t> raw;
  StrId title = StrId::STR_NONE_OPT;
  uint8_t currentRaw = 0;

  if (row == RowId::FontSize) {
    if (draft.sdFontFamilyName[0] != '\0') {
      sdFontSystem.refreshIfDirty();
      if (const auto* family = sdFontSystem.registry().findFamily(draft.sdFontFamilyName.data())) {
        raw = family->availableSizes();
      }
    }
    if (raw.empty()) {
      raw.assign(std::begin(BUILTIN_READER_FONT_SIZES), std::end(BUILTIN_READER_FONT_SIZES));
    }
    labels.reserve(raw.size());
    std::transform(raw.begin(), raw.end(), std::back_inserter(labels),
                   [](const uint8_t size) { return fontSizePointLabel(size); });
    const auto it = std::find(raw.begin(), raw.end(), draft.readerFontPointSize);
    const int current = it == raw.end() ? 0 : static_cast<int>(std::distance(raw.begin(), it));
    return openEnumOptions(row, StrId::STR_FONT_SIZE, std::move(labels), std::move(raw), current);
  }

  switch (row) {
    case RowId::Orientation:
      title = StrId::STR_ORIENTATION;
      labels = {tr(STR_PORTRAIT), tr(STR_LANDSCAPE_CW), tr(STR_LANDSCAPE_CCW), tr(STR_ORIENTATION_INVERTED)};
      raw = {CrossPointSettings::PORTRAIT, CrossPointSettings::LANDSCAPE_CW, CrossPointSettings::LANDSCAPE_CCW,
             CrossPointSettings::INVERTED};
      currentRaw = draft.orientation;
      break;
    case RowId::Alignment:
      title = StrId::STR_PARA_ALIGNMENT;
      labels = {tr(STR_JUSTIFY), tr(STR_DIR_LEFT), tr(STR_CENTER), tr(STR_DIR_RIGHT), tr(STR_BOOK_S_STYLE)};
      raw = {0, 1, 2, 3, 4};
      currentRaw = draft.paragraphAlignment;
      break;
    case RowId::PublisherFonts:
      title = StrId::STR_PUBLISHER_FONTS;
      labels = {tr(STR_OFF), tr(STR_PUBLISHER_FONTS_ACCENTS), tr(STR_PUBLISHER_FONTS_ALL)};
      raw = {0, 1, 2};
      currentRaw = draft.publisherFonts;
      break;
    case RowId::Images:
      title = StrId::STR_IMAGES;
      labels = {tr(STR_IMAGES_DISPLAY), tr(STR_IMAGES_DISPLAY_BW), tr(STR_IMAGES_PLACEHOLDER), tr(STR_IMAGES_SUPPRESS)};
      raw = {ImageDisplayMode::Grayscale, ImageDisplayMode::BlackAndWhite, ImageDisplayMode::Placeholder,
             ImageDisplayMode::Hidden};
      currentRaw = ImageDisplayMode::selected(draft.imageRendering, draft.imageGrayscale);
      break;
    case RowId::RenderMode:
      title = StrId::STR_EPUB_RENDER_MODE;
      labels = {tr(STR_RENDER_MODE_CROSSINK_DEFAULT), tr(STR_RENDER_MODE_BALANCED), tr(STR_RENDER_MODE_LIGHT)};
      raw = {0, 1, 2};
      currentRaw = draft.epubRenderMode;
      break;
    case RowId::DictionaryFontFamily: {
      title = StrId::STR_FONT_FAMILY;
      labels.emplace_back(tr(STR_DICT_USE_GLOBAL));
      raw.emplace_back(0);
      const auto& families = sdFontSystem.registry().getFamilies();
      for (size_t i = 0; i < families.size(); ++i) {
        labels.push_back(families[i].name);
        raw.push_back(static_cast<uint8_t>(i + 1));
      }
      if (hasDictionaryFontOverride && dictionaryFontFamilyName[0] != '\0' &&
          sdFontSystem.registry().findSummary(dictionaryFontFamilyName) == nullptr) {
        labels.push_back(std::string(dictionaryFontFamilyName) + " (" + tr(STR_UNAVAILABLE) + ")");
        raw.push_back(0);
      }
      if (hasDictionaryFontOverride && dictionaryFontFamilyName[0] != '\0') {
        for (size_t i = 1; i < labels.size(); ++i) {
          if (labels[i] == dictionaryFontFamilyName) {
            currentRaw = raw[i];
            break;
          }
        }
      }
      break;
    }
    case RowId::DictionaryFontSize: {
      title = StrId::STR_DICTIONARY_FONT_SIZE;
      labels.emplace_back(tr(STR_DICT_USE_GLOBAL));
      raw.emplace_back(0);
      const char* familyName =
          dictionaryFontFamilyName[0] != '\0' ? dictionaryFontFamilyName : SETTINGS.sdFontFamilyName;
      if (familyName[0] != '\0') {
        if (const auto* family = sdFontSystem.registry().findFamily(familyName)) {
          const auto sizes = family->availableSizes();
          labels.reserve(sizes.size() + 1);
          raw.reserve(sizes.size() + 1);
          for (const uint8_t size : sizes) {
            labels.push_back(fontSizePointLabel(size));
            raw.push_back(size);
          }
        }
      }
      currentRaw = hasDictionaryFontOverride ? dictionaryFontPointSize : 0;
      break;
    }
    default:
      return false;
  }
  int current = 0;
  if (row == RowId::DictionaryFontFamily && hasDictionaryFontOverride && dictionaryFontFamilyName[0] != '\0' &&
      sdFontSystem.registry().findSummary(dictionaryFontFamilyName) == nullptr) {
    current = static_cast<int>(labels.size()) - 1;
  } else {
    const auto rawIt = std::find(raw.begin(), raw.end(), currentRaw);
    if (rawIt != raw.end()) current = static_cast<int>(std::distance(raw.begin(), rawIt));
  }
  return openEnumOptions(row, title, std::move(labels), std::move(raw), current);
}

bool ReaderSettingsController::openEnumOptions(const RowId row, const StrId title, std::vector<std::string> labels,
                                               std::vector<uint8_t> values, const int selectedIndex) {
  if (labels.empty() || labels.size() != values.size()) return false;
  enumOptionRow = row;
  enumOptionTitle = title;
  enumOptionLabels = std::move(labels);
  enumOptionValues = std::move(values);
  enumOptionSelectedIndex =
      static_cast<int16_t>(std::clamp(selectedIndex, 0, static_cast<int>(enumOptionLabels.size()) - 1));
  previewedEnumOptionIndex = -1;
  return true;
}

void ReaderSettingsController::notifyDictionaryFontChanged() {
  if (callbacks.dictionaryFontChanged) {
    callbacks.dictionaryFontChanged(callbacks.dictionaryFontChangedContext,
                                    hasDictionaryFontOverride ? dictionaryFontFamilyName : nullptr,
                                    dictionaryFontPointSize);
  }
}

ReaderSettingsController::EnumSelection ReaderSettingsController::selectEnumOption(const int index) {
  if (index < 0 || index >= static_cast<int>(enumOptionValues.size())) return EnumSelection::Ignored;
  const uint8_t value = enumOptionValues[static_cast<size_t>(index)];
  if (readerEnumOptionPreviewsFirst(enumOptionRow) && previewedEnumOptionIndex != index) {
    previewedEnumOptionIndex = static_cast<int16_t>(index);
    if (enumOptionRow == RowId::FontSize) {
      fontPreviewLoading = ReaderUtils::shouldShowFontPreviewLoading(draft.sdFontFamilyName.data());
    }
    markSettingChanged(applyReaderEnumOption(draft, enumOptionRow, value));
    return EnumSelection::Previewed;
  }
  switch (enumOptionRow) {
    case RowId::FontSize:
    case RowId::Alignment:
      enumOptionSelectedIndex = static_cast<int16_t>(index);
      previewedEnumOptionIndex = -1;
      markSettingChanged(applyReaderEnumOption(draft, enumOptionRow, value));
      break;
    case RowId::Orientation:
    case RowId::PublisherFonts:
    case RowId::Images:
    case RowId::RenderMode:
      markSettingChanged(applyReaderEnumOption(draft, enumOptionRow, value));
      break;
    case RowId::DictionaryFontFamily:
      if (index == 0) {
        hasDictionaryFontOverride = false;
        std::strncpy(dictionaryFontFamilyName, SETTINGS.dictionarySdFontFamilyName,
                     sizeof(dictionaryFontFamilyName) - 1);
        dictionaryFontFamilyName[sizeof(dictionaryFontFamilyName) - 1] = '\0';
        dictionaryFontPointSize = SETTINGS.dictionaryFontPointSize;
      } else if (const auto* family =
                     sdFontSystem.registry().findFamily(enumOptionLabels[static_cast<size_t>(index)].c_str())) {
        hasDictionaryFontOverride = true;
        std::strncpy(dictionaryFontFamilyName, family->name.c_str(), sizeof(dictionaryFontFamilyName) - 1);
        dictionaryFontFamilyName[sizeof(dictionaryFontFamilyName) - 1] = '\0';
      }
      notifyDictionaryFontChanged();
      break;
    case RowId::DictionaryFontSize:
      if (!hasDictionaryFontOverride) {
        if (index == 0) {
          dictionaryFontPointSize = SETTINGS.dictionaryFontPointSize;
        } else if (dictionaryFontFamilyName[0] != '\0') {
          hasDictionaryFontOverride = true;
          dictionaryFontPointSize = value;
        } else if (SETTINGS.sdFontFamilyName[0] != '\0') {
          std::strncpy(dictionaryFontFamilyName, SETTINGS.sdFontFamilyName, sizeof(dictionaryFontFamilyName) - 1);
          dictionaryFontFamilyName[sizeof(dictionaryFontFamilyName) - 1] = '\0';
          hasDictionaryFontOverride = true;
          dictionaryFontPointSize = value;
        }
      } else {
        dictionaryFontPointSize = value;
      }
      notifyDictionaryFontChanged();
      break;
#if CROSSINK_SCALABLE_FONTS
    case RowId::TtfHinting:
    case RowId::TtfRaster:
    case RowId::TtfInterpreter:
    case RowId::TtfWeight:
    case RowId::TtfSlant: {
      using TtfRow = TtfRenderOptionsActivity::Row;
      const TtfRow optionRow = enumOptionRow == RowId::TtfHinting       ? TtfRow::Hinting
                               : enumOptionRow == RowId::TtfRaster      ? TtfRow::Raster
                               : enumOptionRow == RowId::TtfInterpreter ? TtfRow::Interpreter
                               : enumOptionRow == RowId::TtfWeight      ? TtfRow::Weight
                                                                        : TtfRow::Slant;
      TtfRenderOptionsActivity::setOption(ttfRenderProfile, optionRow, value);
      saveTtfRenderingProfile();
      break;
    }
#endif
    default:
      return EnumSelection::Ignored;
  }
  return EnumSelection::Selected;
}

bool ReaderSettingsController::saveBookDictionary(const Epub* epub, const std::string& path) {
  if (!epub) return false;
  HalFile file;
  const std::string target = epub->getCachePath() + "/dictionary.bin";
  if (!Storage.openFileForWrite("ERDM", target, file)) {
    LOG_ERR("ERDM", "Could not save per-book dictionary");
    return false;
  }
  const bool ok =
      path.empty() || file.write(reinterpret_cast<const uint8_t*>(path.c_str()), path.size()) == path.size();
  file.close();
  if (!ok) LOG_ERR("ERDM", "Short write saving per-book dictionary");
  return ok;
}

ReaderSettingsController::PreviewFont ReaderSettingsController::loadPreviewFont(
    GfxRenderer& renderer, const ReaderSettingsDraft& previewSettings) {
  // The font ID comes from SETTINGS, so swap the draft in only while resolving it.
  const ReaderSettingsDraft liveSettings = captureSettings();
  applySettings(previewSettings);
  sdFontSystem.ensureLoaded(renderer);
  PreviewFont font;
  font.fontId = SETTINGS.getReaderFontId();
  font.loaded =
      previewSettings.sdFontFamilyName[0] == '\0' ||
      sdFontSystem.resolveFontId(previewSettings.sdFontFamilyName.data(), previewSettings.readerFontPointSize) != 0;
  applySettings(liveSettings);
  return font;
}
