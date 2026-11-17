#pragma once

#include <cstdint>

#include "EpubReaderMenuModel.h"
#include "util/ImageDisplayMode.h"

// Pure reader-settings draft edits behind ReaderSettingsController. No SETTINGS,
// storage or rendering here, so the native tests cover them directly.

struct ReaderSettingsChangeState {
  // Draft edits not yet written back to SETTINGS.
  bool settingsChanged = false;
  // Any edit since the menu opened, kept after a commit for the reader's result.
  bool didChangeSettings = false;
  ReaderSettingsChangeMask changeMask = ReaderSettingsChangeMask::None;

  void mark(const ReaderSettingsChangeMask mask) {
    settingsChanged = true;
    didChangeSettings = true;
    changeMask = changeMask | mask;
  }
};

// Flips a toggle row in the draft. Returns the change it causes, or None for a row that is not a draft toggle.
inline ReaderSettingsChangeMask toggleReaderSetting(ReaderSettingsDraft& draft, const ReaderDrawerCatalogItem row) {
  using RowId = ReaderDrawerCatalogItem;
  switch (row) {
    case RowId::TextAa:
      draft.textAntiAliasing = !draft.textAntiAliasing;
      break;
    case RowId::Focus:
      draft.focusReadingEnabled = !draft.focusReadingEnabled;
      break;
    case RowId::GuideDots:
      draft.guideReadingEnabled = !draft.guideReadingEnabled;
      break;
    case RowId::Hyphenation:
      draft.hyphenationEnabled = !draft.hyphenationEnabled;
      break;
    case RowId::PublisherPages:
      draft.publisherPageNumbers = !draft.publisherPageNumbers;
      break;
    case RowId::ExtraSpacing:
      draft.extraParagraphSpacing = !draft.extraParagraphSpacing;
      break;
    case RowId::ForceIndents:
      draft.forceParagraphIndents = !draft.forceParagraphIndents;
      break;
    case RowId::EmbeddedStyle:
      draft.embeddedStyle = !draft.embeddedStyle;
      break;
    case RowId::IndexingMethod:
      draft.indexingMethod = draft.indexingMethod == 0 ? 1 : 0;
      break;
    default:
      return ReaderSettingsChangeMask::None;
  }
  if (row == RowId::TextAa) {
    // Anti-aliasing changes pixels only. Rebuilding the EPUB section here
    // makes the in-drawer preview appear to zoom while the page reflows.
    return ReaderSettingsChangeMask::Preview | ReaderSettingsChangeMask::NonLayout;
  }
  const bool previews = row == RowId::Focus || row == RowId::GuideDots;
  return previews ? ReaderSettingsChangeMask::Preview | ReaderSettingsChangeMask::Relayout
                  : ReaderSettingsChangeMask::Relayout;
}

// Rows whose first tap on an option only previews it; a second tap on the same option selects it.
constexpr bool readerEnumOptionPreviewsFirst(const ReaderDrawerCatalogItem row) {
  return row == ReaderDrawerCatalogItem::FontSize || row == ReaderDrawerCatalogItem::Alignment;
}

// Writes a draft-backed option row. Returns the change it causes, or None for a row the draft does not own.
inline ReaderSettingsChangeMask applyReaderEnumOption(ReaderSettingsDraft& draft, const ReaderDrawerCatalogItem row,
                                                      const uint8_t value) {
  using RowId = ReaderDrawerCatalogItem;
  switch (row) {
    case RowId::FontSize:
      draft.readerFontPointSize = value;
      return ReaderSettingsChangeMask::Preview | ReaderSettingsChangeMask::Relayout;
    case RowId::Orientation:
      draft.orientation = value;
      return ReaderSettingsChangeMask::Orientation;
    case RowId::Alignment:
      draft.paragraphAlignment = value;
      return ReaderSettingsChangeMask::Preview | ReaderSettingsChangeMask::Relayout;
    case RowId::PublisherFonts:
      draft.publisherFonts = value;
      return ReaderSettingsChangeMask::Relayout;
    case RowId::Images:
      ImageDisplayMode::apply(value, draft.imageRendering, draft.imageGrayscale);
      return ReaderSettingsChangeMask::NonLayout;
    case RowId::RenderMode:
      draft.epubRenderMode = value;
      return ReaderSettingsChangeMask::Relayout;
    default:
      return ReaderSettingsChangeMask::None;
  }
}

enum class ReaderSettingsCommit : uint8_t { Unchanged, Save, SaveWithPreviousFont };

// Decides what a commit writes. With requirePreviewedSdFont (button devices), an SD font
// whose preview never loaded falls back to the last previewed font; other edits stay.
inline ReaderSettingsCommit prepareReaderSettingsCommit(const bool settingsChanged, const bool requirePreviewedSdFont,
                                                        ReaderSettingsDraft& draft,
                                                        const ReaderSettingsDraft& lastGoodPreview) {
  if (!settingsChanged) return ReaderSettingsCommit::Unchanged;
  if (requirePreviewedSdFont && draft.sdFontFamilyName[0] != '\0' &&
      (draft.sdFontFamilyName != lastGoodPreview.sdFontFamilyName ||
       draft.readerFontPointSize != lastGoodPreview.readerFontPointSize)) {
    restoreReaderDraftFont(draft, lastGoodPreview);
    return ReaderSettingsCommit::SaveWithPreviousFont;
  }
  return ReaderSettingsCommit::Save;
}
