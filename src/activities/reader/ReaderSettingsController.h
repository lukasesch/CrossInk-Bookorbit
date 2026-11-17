#pragma once

#include <I18n.h>

#include <cstdint>
#include <string>
#include <vector>

#include "EpubReaderMenuModel.h"
#include "ReaderSettingsEdits.h"
#if CROSSINK_SCALABLE_FONTS
#include "TtfRenderProfileStore.h"
#endif

class Epub;
class GfxRenderer;

// Owns a reader menu's settings draft: editing, the font / option / dictionary pickers'
// data, live-preview bookkeeping and the commit back to SETTINGS. It speaks in catalog
// rows; panes, focus, drawing and navigation stay with the menu that hosts it. Methods
// report what happened so the host decides which pane to show and when to redraw.
class ReaderSettingsController {
 public:
  using RowId = ReaderDrawerCatalogItem;
  using SaveSettingsCallback = void (*)(void* ctx);
  using SaveGlobalSettingsCallback = void (*)(void* ctx);
  using GlobalSettingsEditCallback = void (*)(void* ctx);
  using DictionaryFontChangedCallback = void (*)(void* ctx, const char* familyName, uint8_t pointSize);

  struct Callbacks {
    SaveSettingsCallback saveReaderSettings = nullptr;
    void* saveReaderSettingsContext = nullptr;
    SaveGlobalSettingsCallback saveGlobalSettings = nullptr;
    void* saveGlobalSettingsContext = nullptr;
    GlobalSettingsEditCallback beginGlobalSettingsEdit = nullptr;
    void* beginGlobalSettingsEditContext = nullptr;
    GlobalSettingsEditCallback endGlobalSettingsEdit = nullptr;
    void* endGlobalSettingsEditContext = nullptr;
    DictionaryFontChangedCallback dictionaryFontChanged = nullptr;
    void* dictionaryFontChangedContext = nullptr;
  };

  enum class EnumSelection : uint8_t { Ignored, Previewed, Selected };

  struct PreviewFont {
    int fontId = -1;
    bool loaded = false;
  };

  ReaderSettingsController(const ReaderSettingsDraft& current, const Callbacks& callbacks,
                           const char* dictionaryFontFamilyName, uint8_t dictionaryFontPointSize,
                           bool hasDictionaryFontOverride);

  static ReaderSettingsDraft captureSettings();
  static void applySettings(const ReaderSettingsDraft& settings);

  void markSettingChanged(ReaderSettingsChangeMask mask);
  // False for a row that is not a draft toggle.
  bool toggleSetting(RowId row);
  ReaderSettingsCommit commitSettings(bool requirePreviewedSdFont);
  // Frees the font and dictionary catalogs when the menu closes.
  void releaseCatalogs();

  // Global settings sub-screens (status bar, controls) opened from the menu.
  void beginGlobalSettingsEdit() const;
  void endGlobalSettingsEdit() const;
  void saveGlobalSettings() const;

  // Font family picker.
  void discoverFonts(GfxRenderer& renderer);
  int currentFontSelectionIndex(int pendingFontIndex) const;
  // Puts the picked family in the draft for preview. False for an out-of-range index.
  bool previewFontFamily(int index);

  // One-tap edits for the Picco sheet's steppers, segmented rows and font chips.
  // Each returns false when the value did not change.
  enum class Step : uint8_t { FontSize, LineSpacing, WordSpacing, CharacterSpacing, MarginTopBottom, MarginSides };
  bool step(Step what, int delta);
  bool setEnumValue(RowId row, uint8_t value);
  bool selectBuiltinFontFamily(uint8_t fontFamily);

  // Option picker. show/open return false when there is nothing to pick.
  bool showEnumOptions(GfxRenderer& renderer, RowId row);
  bool openEnumOptions(RowId row, StrId title, std::vector<std::string> labels, std::vector<uint8_t> values,
                       int selectedIndex);
  EnumSelection selectEnumOption(int index);

  // Per-book dictionary picker.
  void discoverDictionaries(const Epub* epub);
  bool selectBookDictionary(const Epub* epub, int index);

#if CROSSINK_SCALABLE_FONTS
  void beginTtfRenderingEdit();
  bool saveTtfRenderingProfile();
  void finishTtfRenderingEdit(GfxRenderer& renderer);
  bool showTtfRenderingOptions(RowId row);
  void toggleTtfStemDarkening();
  // Restores default rendering; false (profile unchanged) if it could not be saved.
  bool resetTtfRenderingProfile();
#endif

  // Live preview hooks.
  static PreviewFont loadPreviewFont(GfxRenderer& renderer, const ReaderSettingsDraft& previewSettings);
  void restoreLastGoodPreviewFont() { restoreReaderDraftFont(draft, lastGoodPreviewSettings); }
  void notePreviewRendered() { lastGoodPreviewSettings = draft; }

  ReaderSettingsDraft draft{};
  const ReaderSettingsDraft sourceSettings;
  ReaderSettingsDraft lastGoodPreviewSettings{};
  ReaderSettingsChangeState changes{};
  // Set when an edit needs the preview rebuilt; the host clears it once drawn.
  bool previewDirty = false;
  bool previewFontMetricsChanged = false;
  bool fontPreviewLoading = false;

  std::vector<std::string> fontLabels;
  std::vector<uint8_t> fontSettingIndexes;
  std::vector<std::string> enumOptionLabels;
  std::vector<uint8_t> enumOptionValues;
  RowId enumOptionRow = RowId::FontSize;
  StrId enumOptionTitle = StrId::STR_NONE_OPT;
  int16_t enumOptionSelectedIndex = 0;
  int16_t previewedEnumOptionIndex = -1;
  std::vector<std::string> dictionaryLabels;
  std::vector<std::string> dictionaryPaths;
  std::string bookDictionaryPath;
  char dictionaryFontFamilyName[64] = {};
  uint8_t dictionaryFontPointSize = 0;
  bool hasDictionaryFontOverride = false;
#if CROSSINK_SCALABLE_FONTS
  TtfRenderProfile ttfRenderProfile{};
  TtfRenderProfile initialTtfRenderProfile{};
  bool ttfRenderingChanged = false;
#endif

 private:
  Callbacks callbacks;

  void notifyDictionaryFontChanged();
  bool saveBookDictionary(const Epub* epub, const std::string& path);
};
