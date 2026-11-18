// The Picco reader sheet: FrontlightPanelActivity under the PICCO theme in portrait.
// Board T_ReaderSheetDivider and its panes; geometry lives in PiccoSheetModel.h.
#include "FrontlightPanelActivity.h"

#if CROSSINK_APP_PICCO_UI

#include <CrossInkHalFrontlight.h>
#include <GfxRenderer.h>
#include <I18n.h>
#include <Logging.h>
#include <Memory.h>
#include <SdCardFontSystem.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>

#include "BookOrbitCredentialStore.h"
#include "BookOrbitHomeStore.h"
#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "SilentRestart.h"
#include "activities/ActivityManager.h"
#include "activities/RenderLock.h"
#include "activities/reader/ReaderSettingsController.h"
#include "activities/reader/XtcReaderMenuActivity.h"
#include "components/UITheme.h"
#include "components/icons/piccoIcons.h"
#include "components/icons/piccoSheetIcons.h"
#include "components/picco/PiccoPillButton.h"
#include "components/picco/PiccoProgress.h"
#include "components/picco/PiccoSwitch.h"
#include "components/picco/PiccoTheme.h"
#include "components/picco/home/PiccoHomeStops.h"
#include "components/picco/sheet/PiccoSheetDraw.h"
#include "components/picco/sheet/ReaderSheetDetails.h"
#include "util/QrUtils.h"
#include "fontIds.h"
#include "util/ImageDisplayMode.h"
#include "util/SlideSwitch.h"
#if CROSSINK_SCALABLE_FONTS
#include "TtfRenderProfileStore.h"
#include "activities/settings/TtfRenderOptionsActivity.h"
#endif

namespace {
using PiccoLayout::Box;
using namespace PiccoSheet;
namespace D = PiccoSheetDraw;
using RowId = ReaderDrawerCatalogItem;
using Hit = FrontlightPanelActivity;

constexpr int kStepLineSpacing = 5;
constexpr int kStepMargin = 2;
constexpr int kAutoTurnStep = 5;
constexpr int kLightButtonStep = 5;
constexpr int kStepperReserve = 2 * PiccoTokens::kStepperButtonW + kStepperValueW + 2 * PiccoTokens::kNeighbourGap;

Rect toRect(const Box& b) { return Rect{b.x, b.y, b.width, b.height}; }

// Rows the settings controller edits, by its catalog id.
bool controllerRow(const Row row, RowId& out) {
  switch (row) {
    case Row::Hyphenation:
      out = RowId::Hyphenation;
      return true;
    case Row::ForceIndents:
      out = RowId::ForceIndents;
      return true;
    case Row::ExtraSpacing:
      out = RowId::ExtraSpacing;
      return true;
    case Row::Focus:
      out = RowId::Focus;
      return true;
    case Row::GuideDots:
      out = RowId::GuideDots;
      return true;
    case Row::TextAa:
      out = RowId::TextAa;
      return true;
    case Row::EmbeddedStyle:
      out = RowId::EmbeddedStyle;
      return true;
    case Row::PublisherPages:
      out = RowId::PublisherPages;
      return true;
    case Row::IndexingMethod:
      out = RowId::IndexingMethod;
      return true;
    case Row::PublisherFonts:
      out = RowId::PublisherFonts;
      return true;
    case Row::Images:
      out = RowId::Images;
      return true;
    case Row::RenderMode:
      out = RowId::RenderMode;
      return true;
    case Row::DictionaryFont:
      out = RowId::DictionaryFontFamily;
      return true;
    case Row::DictionaryFontSize:
      out = RowId::DictionaryFontSize;
      return true;
    case Row::Alignment:
      out = RowId::Alignment;
      return true;
    case Row::Orientation:
      out = RowId::Orientation;
      return true;
    case Row::TtfHinting:
      out = RowId::TtfHinting;
      return true;
    case Row::TtfRaster:
      out = RowId::TtfRaster;
      return true;
    case Row::TtfInterpreter:
      out = RowId::TtfInterpreter;
      return true;
    case Row::TtfWeight:
      out = RowId::TtfWeight;
      return true;
    case Row::TtfSlant:
      out = RowId::TtfSlant;
      return true;
    default:
      return false;
  }
}

// Segmented rows in board order: Left, Center, Justified (+ Right, Book style) and the
// four orientations. Values are the settings' raw values.
constexpr uint8_t kAlignmentValues[] = {1, 2, 0, 3, 4};
constexpr uint8_t kOrientationValues[] = {CrossPointSettings::PORTRAIT, CrossPointSettings::LANDSCAPE_CW,
                                          CrossPointSettings::INVERTED, CrossPointSettings::LANDSCAPE_CCW};
const freeink::Icon* const kAlignmentIcons[] = {&icon_picco_sheet_align_left_24, &icon_picco_sheet_align_center_24,
                                                &icon_picco_sheet_align_justify_24, &icon_picco_sheet_align_right_24,
                                                &icon_picco_sheet_align_book_24};
const freeink::Icon* const kOrientationIcons[] = {&icon_picco_sheet_portrait_24, &icon_picco_sheet_landscape_cw_24,
                                                  &icon_picco_sheet_portrait_inverted_24,
                                                  &icon_picco_sheet_landscape_ccw_24};

ReaderSettingsController::Step stepFor(const Row row) {
  using Step = ReaderSettingsController::Step;
  switch (row) {
    case Row::FontSize:
      return Step::FontSize;
    case Row::LineSpacing:
      return Step::LineSpacing;
    case Row::WordSpacing:
      return Step::WordSpacing;
    case Row::CharacterSpacing:
      return Step::CharacterSpacing;
    case Row::TopBottomMargins:
      return Step::MarginTopBottom;
    default:
      return Step::MarginSides;
  }
}

int stepSize(const Row row) {
  switch (row) {
    case Row::LineSpacing:
      return kStepLineSpacing;
    case Row::SideMargins:
    case Row::TopBottomMargins:
      return kStepMargin;
    default:
      return 1;
  }
}

const char* sectionLabel(const Section section) {
  switch (section) {
    case Section::Font:
      return tr(STR_PICCO_SHEET_FONT);
    case Section::Paragraphs:
      return tr(STR_PICCO_SHEET_PARAGRAPHS);
    case Section::Style:
      return tr(STR_PICCO_SHEET_STYLE);
    case Section::Spacing:
      return tr(STR_PICCO_SHEET_SPACING);
    case Section::BookStyles:
      return tr(STR_PICCO_SHEET_BOOK_STYLES);
    case Section::Navigate:
      return tr(STR_PICCO_SHEET_NAVIGATE);
    case Section::Words:
      return tr(STR_PICCO_SHEET_WORDS);
    case Section::SyncShare:
      return tr(STR_PICCO_SHEET_SYNC_SHARE);
    case Section::ThisBook:
      return tr(STR_PICCO_SHEET_THIS_BOOK);
    case Section::Book:
      return tr(STR_BOOK);
    case Section::Reset:
      return tr(STR_PICCO_SHEET_RESET);
    case Section::None:
      break;
  }
  return "";
}

const char* paneTitle(const Pane pane) {
  switch (pane) {
    case Pane::Text:
      return tr(STR_PICCO_SHEET_TEXT);
    case Pane::TextMore:
      return tr(STR_PICCO_SHEET_MORE_TEXT);
    case Pane::More:
      return tr(STR_MORE);
    case Pane::GoTo:
      return tr(STR_PICCO_SHEET_GO_TO);
    case Pane::AutoPageTurn:
      return tr(STR_AUTO_TURN_INTERVAL_SECONDS);
    case Pane::Advanced:
      return tr(STR_SYSTEM_ADVANCED);
    case Pane::FontRendering:
      return tr(STR_TTF_RENDERING);
    case Pane::Qr:
      return tr(STR_PICCO_QR_TITLE);
    case Pane::Main:
    case Pane::Count:
      break;
  }
  return tr(STR_HOME);
}

const char* builtinFontName(const uint8_t family) { return family == 0 ? tr(STR_LEXEND_DECA) : tr(STR_BITTER); }

#if CROSSINK_SCALABLE_FONTS
TtfRenderOptionsActivity::Row ttfRow(const Row row) {
  using T = TtfRenderOptionsActivity::Row;
  switch (row) {
    case Row::TtfHinting:
      return T::Hinting;
    case Row::TtfRaster:
      return T::Raster;
    case Row::TtfInterpreter:
      return T::Interpreter;
    case Row::TtfWeight:
      return T::Weight;
    case Row::TtfSlant:
      return T::Slant;
    case Row::TtfStemDarkening:
      return T::StemDarkening;
    default:
      return T::Reset;
  }
}
#endif
}  // namespace

// ---- Lifecycle -------------------------------------------------------------------------------

void FrontlightPanelActivity::sheetEnter() {
  const ReaderSheetDetails* details = context.readerSheet.get();
  if (details && details->format == ReaderSheetDetails::Format::Epub) {
    sheetVariant = Variant::Epub;
    sheetSettings = makeUniqueNoThrow<ReaderSettingsController>(
        ReaderSettingsController::captureSettings(), details->callbacks, details->dictionaryFontFamily.c_str(),
        details->dictionaryFontPointSize, details->hasDictionaryFontOverride);
    if (!sheetSettings) {
      LOG_ERR("SHEET", "OOM: reader settings; the sheet shows light and actions only");
      sheetVariant = Variant::Reader;
    }
    autoTurnSeconds = details->autoPageTurnSeconds;
  } else if (context.activeReaderBook) {
    sheetVariant = Variant::Reader;
  }
  const FrontlightDrawerState& start = details ? details->initialState : drawerState;
  if (sheetVariant == Variant::Epub && start.sheetPane < static_cast<uint8_t>(Pane::Count)) {
    sheetSetPane(static_cast<Pane>(start.sheetPane));
    sheetTab = static_cast<uint8_t>(std::min<int>(start.sheetTab, tabCount(sheetPane, true) - 1));
    sheetFocus = start.sheetFocus;
    sheetRing = sheetFocus >= 0;
  }

  sheetHasBookOrbit = BOOKORBIT_STORE.hasCredentials();
  sheetOffline = homeSyncWentOffline();
  if (sheetHasBookOrbit) {
    auto snapshot = makeUniqueNoThrow<BookOrbitHomeSnapshot>();
    if (snapshot && BookOrbitHomeStore::load(*snapshot)) sheetSyncedAt = snapshot->generatedAt;
  }

  // One copy of the page under the sheet, so pane switches repaint it in the same refresh.
  const size_t size = renderer.getBufferSize();
  sheetBackdrop = psramHeapAvailable() ? makePsramByteBufferNoThrow(size) : makeHeapByteBufferNoThrow(size);
  if (!sheetBackdrop) LOG_ERR("SHEET", "No memory for the backdrop copy; the sheet will not shrink");
  sheetBackdropSaved = false;
}

void FrontlightPanelActivity::sheetExit() {
  if (sheetSettings) {
#if CROSSINK_SCALABLE_FONTS
    sheetSettings->finishTtfRenderingEdit(renderer);
#endif
    // Leaving without a result (Home, sleep) still keeps the edits, like the drawer.
    sheetSettings->commitSettings(false);
    sheetSettings->releaseCatalogs();
    sheetSettings.reset();
  }
  sheetBackdrop.reset();
  sheetQrText = std::string();
}

void FrontlightPanelActivity::sheetFinish(const ReaderSheetRequest& request, const FrontlightPanelAction action) {
  ReaderSheetRequest out = request;
  out.fromSheet = true;
  out.orientation = SETTINGS.orientation;
  if (sheetSettings) {
#if CROSSINK_SCALABLE_FONTS
    sheetSettings->finishTtfRenderingEdit(renderer);
#endif
    out.settingsChanged = sheetSettings->changes.didChangeSettings;
    sheetSettings->commitSettings(false);
    out.changeMask = static_cast<uint8_t>(sheetSettings->changes.changeMask);
    out.orientation = sheetSettings->draft.orientation;
  }
  drawerState.sheetPane = static_cast<uint8_t>(sheetPane);
  drawerState.sheetTab = sheetTab;
  drawerState.sheetFocus = static_cast<int8_t>(sheetRing ? sheetFocus : -1);
  FrontlightPanelResult result;
  result.action = action;
  result.state = drawerState;
  result.activeEpub = context.activeEpub;
  result.ttfRenderingChanged = ttfRenderingChanged;
  result.bookPath = context.bookPath;
  result.sheet = out;
  setResult(ActivityResult(std::move(result)));
  finish();
}

void FrontlightPanelActivity::sheetRequest(const int16_t action, const int32_t value, const uint32_t page,
                                           const bool reopen) {
  ReaderSheetRequest request;
  request.action = action;
  request.value = value;
  request.page = page;
  request.reopen = reopen;
  sheetFinish(request);
}

// ---- State -----------------------------------------------------------------------------------

Availability FrontlightPanelActivity::sheetAvailability() const {
  Availability a;
  const ReaderSheetDetails* d = context.readerSheet.get();
  if (!d) return a;
  a.hasFootnotes = d->hasFootnotes;
  a.hasDictionary = d->hasDictionary;
  a.hasBookmarks = d->bookmarkCount > 0;
  a.hasClippings = d->hasClippings;
  a.showReadingPaceReset = d->showReadingPaceReset;
  a.globalStats = d->globalStats;
  a.bookStats = d->bookStats;
#if CROSSINK_APP_CAP_KOREADER_SYNC
  a.koreaderSync = true;
#endif
#if CROSSINK_SCALABLE_FONTS
  a.publisherFonts = true;
  if (sheetSettings) {
    a.fontRendering = sdFontSystem.isScalableFamily(sheetSettings->draft.sdFontFamilyName.data());
    a.ttfInterpreter = sheetSettings->ttfRenderProfile.hinting == 1;
  }
#endif
  return a;
}

Tiles FrontlightPanelActivity::sheetTiles() const {
  const ReaderSheetDetails* d = context.readerSheet.get();
  return tilesFor(sheetVariant, d && d->hasContents, d && d->hasMenu);
}

int FrontlightPanelActivity::sheetHeight() const {
  const int w = renderer.getScreenWidth();
  int bottom = 0;
  switch (sheetPane) {
    case Pane::Main: {
      const bool book = sheetVariant != Variant::Outside;
      bottom = mainLayout(w, book, book && !context.bookDetails.author.empty(), Frontlight.hasColorTemperature(),
                          sheetTiles().count, sheetVariant == Variant::Epub)
                   .bottom;
      break;
    }
    case Pane::GoTo: {
      const ReaderSheetDetails* d = context.readerSheet.get();
      bottom = goToLayout(w, d && d->stablePageCount > 0).bottom;
      break;
    }
    case Pane::AutoPageTurn:
      bottom = autoTurnLayout(w).bottom;
      break;
    case Pane::Qr:
      bottom = qrLayout(w).bottom;
      break;
    default:
      bottom = blockPaneBottom(sheetPane, sheetAvailability(), w);
      break;
  }
  // Leave the dither strip on screen.
  return std::min(bottom, renderer.getScreenHeight() - PiccoTokens::kSheetDitherStrip - 2);
}

void FrontlightPanelActivity::sheetSetPane(const Pane pane) {
  sheetPane = pane;
  sheetTab = 0;
  sheetFocus = sheetRing ? 0 : -1;
  sheetSlider = {};
  const ReaderSheetDetails* d = context.readerSheet.get();
  switch (pane) {
    case Pane::More:
      if (sheetSettings && !dictionariesReady) {
        sheetSettings->discoverDictionaries(d ? d->epub.get() : nullptr);
        dictionariesReady = true;
      }
      break;
    case Pane::GoTo:
      sheetOpenGoTo(!d || d->stablePageCount == 0);
      break;
#if CROSSINK_SCALABLE_FONTS
    case Pane::FontRendering:
      if (sheetSettings) sheetSettings->beginTtfRenderingEdit();
      break;
#endif
    case Pane::Qr:
      // A page without words (an image page) has nothing to encode: stay on More.
      sheetQrText = d && d->pageText ? d->pageText(d->pageTextContext) : std::string();
      if (sheetQrText.empty()) {
        LOG_ERR("SHEET", "No page text for the QR pane");
        sheetPane = Pane::More;
      }
      break;
    default:
      break;
  }
  if (pane != Pane::Qr) sheetQrText = std::string();
  requestUpdate();
}

void FrontlightPanelActivity::sheetOpenGoTo(const bool percent) {
  const ReaderSheetDetails* d = context.readerSheet.get();
  goToPercent = percent || !d || d->stablePageCount == 0;
  goToEntry = GoToEntry{};
  if (goToPercent) {
    goToEntry.value = static_cast<uint32_t>(std::clamp(context.bookDetails.progressPercent, 0, 100));
    goToEntry.minimum = 0;
    goToEntry.maximum = 100;
  } else {
    goToEntry.minimum = 1;
    goToEntry.maximum = d->stablePageCount;
    goToEntry.value = std::clamp<uint32_t>(d->stablePage, 1, d->stablePageCount);
  }
  if (sheetPane == Pane::GoTo) sheetTab = goToPercent && d && d->stablePageCount > 0 ? 1 : 0;
}

void FrontlightPanelActivity::sheetBack() {
  if (sheetPane == Pane::Main) {
    close();
    return;
  }
  const Pane from = sheetPane;
  sheetSetPane(parentPane(from));
  // Return to the tab the pane was opened from.
  if (from == Pane::Advanced) sheetTab = 1;
}

// ---- Rows ------------------------------------------------------------------------------------

const char* FrontlightPanelActivity::sheetRowLabel(const Row row) const {
  const ReaderSheetDetails* d = context.readerSheet.get();
  switch (row) {
    case Row::FontSize:
      return tr(STR_PICCO_SHEET_SIZE);
    case Row::LineSpacing:
      return tr(STR_LINE_SPACING);
    case Row::SideMargins:
      return tr(STR_PICCO_SHEET_MARGINS);
    case Row::Alignment:
      return tr(STR_PICCO_SHEET_ALIGNMENT);
    case Row::Orientation:
      return tr(STR_ORIENTATION);
    case Row::Hyphenation:
      return tr(STR_HYPHENATION);
    case Row::ForceIndents:
      return tr(STR_FORCE_PARAGRAPH_INDENTS);
    case Row::ExtraSpacing:
      return tr(STR_EXTRA_SPACING);
    case Row::Focus:
      return tr(STR_FOCUS_READING);
    case Row::GuideDots:
      return tr(STR_GUIDE_READING);
    case Row::DarkMode:
      return tr(STR_READER_DARK_MODE);
    case Row::TextAa:
      return tr(STR_TEXT_AA);
    case Row::WordSpacing:
      return tr(STR_WORD_SPACING);
    case Row::CharacterSpacing:
      return tr(STR_CHARACTER_SPACING);
    case Row::TopBottomMargins:
      return tr(STR_PICCO_SHEET_TOP_BOTTOM_MARGINS);
    case Row::EmbeddedStyle:
      return tr(STR_EMBEDDED_STYLE);
    case Row::PublisherFonts:
      return tr(STR_PUBLISHER_FONTS);
    case Row::PublisherPages:
      return tr(STR_PUBLISHER_PAGE_NUMBERS);
    case Row::Images:
      return tr(STR_IMAGES);
    case Row::Footnotes:
      return tr(STR_FOOTNOTES);
    case Row::AutoPageTurn:
      return tr(STR_AUTO_TURN_INTERVAL_SECONDS);
    case Row::Bookmarks:
      return tr(STR_BOOKMARKS);
    case Row::Clippings:
      return tr(STR_CLIPPINGS);
    case Row::Lookup:
      return tr(STR_LOOKUP);
    case Row::LookupHistory:
      return tr(STR_LOOKUP_HISTORY);
    case Row::SaveClipping:
      return tr(STR_SAVE_CLIPPING);
    case Row::BookDictionary:
      return tr(STR_BOOK_DICTIONARY);
    case Row::NearbySync:
      return tr(STR_PICCO_SHEET_NEARBY_SYNC);
    case Row::SendBook:
      return tr(STR_PICCO_SHEET_SEND_BOOK);
    case Row::Qr:
      return tr(STR_PICCO_SHEET_QR);
    case Row::KOReaderSync:
      return tr(STR_SYNC_BOOK);
    case Row::MarkFinished:
      return d && d->completed ? tr(STR_MARK_UNFINISHED) : tr(STR_MARK_FINISHED);
    case Row::BookStats:
      return tr(STR_READING_STATS);
    case Row::StatusBar:
      return tr(STR_STATUS_BARS);
    case Row::RenderMode:
      return tr(STR_PICCO_SHEET_RENDER_MODE);
    case Row::Controls:
      return tr(STR_CAT_CONTROLS);
    case Row::Screenshot:
      return tr(STR_SCREENSHOT_BUTTON);
    case Row::Advanced:
      return tr(STR_SYSTEM_ADVANCED);
    case Row::IndexingMethod:
      return tr(STR_INDEXING_METHOD);
    case Row::TrackStats:
      return tr(STR_TRACK_READING_STATS);
    case Row::DictionaryFont:
      return tr(STR_DICTIONARY_FONT);
    case Row::DictionaryFontSize:
      return tr(STR_DICTIONARY_FONT_SIZE);
    case Row::FontRendering:
      return tr(STR_TTF_RENDERING);
    case Row::ResetPace:
      return tr(STR_RESET_READING_PACE);
    case Row::DeleteStats:
      return tr(STR_DELETE_BOOK_STATS);
    case Row::DeleteBookmarks:
      return tr(STR_DELETE_BOOKMARKS);
    case Row::DeleteCache:
      return tr(STR_DELETE_CACHE);
    case Row::ResetBookSettings:
      return tr(STR_RESET_BOOK_READER_SETTINGS);
#if CROSSINK_SCALABLE_FONTS
    case Row::TtfHinting:
    case Row::TtfRaster:
    case Row::TtfInterpreter:
    case Row::TtfWeight:
    case Row::TtfSlant:
    case Row::TtfStemDarkening:
    case Row::TtfReset:
      return TtfRenderOptionsActivity::rowLabel(ttfRow(row));
#endif
    default:
      return "";
  }
}

const char* FrontlightPanelActivity::sheetRowHint(const Row row) const {
  switch (row) {
    case Row::Focus:
      return tr(STR_PICCO_SHEET_FOCUS_HINT);
    case Row::DarkMode:
      return tr(STR_PICCO_SHEET_DARK_HINT);
    case Row::EmbeddedStyle:
      return tr(STR_PICCO_SHEET_EMBEDDED_HINT);
    case Row::ResetPace:
      return tr(STR_PICCO_SHEET_PACE_HINT);
    case Row::DeleteStats:
      return tr(STR_PICCO_SHEET_STATS_HINT);
    case Row::DeleteBookmarks:
      return tr(STR_PICCO_SHEET_BOOKMARKS_HINT);
    case Row::DeleteCache:
      return tr(STR_PICCO_SHEET_CACHE_HINT);
    case Row::ResetBookSettings:
      return tr(STR_PICCO_SHEET_SETTINGS_HINT);
    default:
      return nullptr;
  }
}

const char* FrontlightPanelActivity::sheetRowValue(const Row row, char* buffer, const size_t size) const {
  const ReaderSheetDetails* d = context.readerSheet.get();
  if (!sheetSettings) return nullptr;
  const ReaderSettingsDraft& draft = sheetSettings->draft;
  switch (row) {
    case Row::FontSize:
      std::snprintf(buffer, size, "%u", draft.readerFontPointSize);
      return buffer;
    case Row::LineSpacing: {
      // 135 % reads as 1.35, 140 % as 1.4, like the board.
      const unsigned whole = draft.lineHeightPercent / 100;
      const unsigned frac = draft.lineHeightPercent % 100;
      if (frac % 10 == 0) {
        std::snprintf(buffer, size, "%u.%u", whole, frac / 10);
      } else {
        std::snprintf(buffer, size, "%u.%02u", whole, frac);
      }
      return buffer;
    }
    case Row::SideMargins:
      std::snprintf(buffer, size, "%u", draft.screenMarginHorizontal);
      return buffer;
    case Row::TopBottomMargins:
      std::snprintf(buffer, size, "%u", draft.screenMarginVertical);
      return buffer;
    case Row::WordSpacing: {
      const int level = WordSpacing::level(draft.wordSpacing);
      std::snprintf(buffer, size, level > 0 ? "+%d" : "%d", level);
      return buffer;
    }
    case Row::CharacterSpacing:
      CrossPointSettings::formatCharacterSpacing(draft.characterSpacing, buffer, size);
      return buffer;
    case Row::PublisherFonts: {
      static constexpr StrId labels[] = {StrId::STR_OFF, StrId::STR_PUBLISHER_FONTS_ACCENTS,
                                         StrId::STR_PUBLISHER_FONTS_ALL};
      return I18N.get(labels[std::min<size_t>(draft.publisherFonts, 2)]);
    }
    case Row::Images: {
      static constexpr StrId labels[] = {StrId::STR_IMAGES_DISPLAY, StrId::STR_IMAGES_DISPLAY_BW,
                                         StrId::STR_IMAGES_PLACEHOLDER, StrId::STR_IMAGES_SUPPRESS};
      return I18N.get(
          labels[std::min<size_t>(ImageDisplayMode::selected(draft.imageRendering, draft.imageGrayscale), 3)]);
    }
    case Row::RenderMode: {
      static constexpr StrId labels[] = {StrId::STR_RENDER_MODE_CROSSINK_DEFAULT, StrId::STR_RENDER_MODE_BALANCED,
                                         StrId::STR_RENDER_MODE_LIGHT};
      return I18N.get(labels[std::min<size_t>(draft.epubRenderMode, 2)]);
    }
    case Row::IndexingMethod:
      return draft.indexingMethod == 0 ? tr(STR_INDEXING_INCREMENTAL) : tr(STR_INDEXING_FULL_SECTION);
    case Row::DictionaryFont:
      return sheetSettings->hasDictionaryFontOverride ? sheetSettings->dictionaryFontFamilyName
                                                      : tr(STR_DICT_USE_GLOBAL);
    case Row::DictionaryFontSize:
      if (!sheetSettings->hasDictionaryFontOverride || sheetSettings->dictionaryFontPointSize == 0)
        return tr(STR_DICT_USE_GLOBAL);
      std::snprintf(buffer, size, "%upt", sheetSettings->dictionaryFontPointSize);
      return buffer;
    case Row::BookDictionary: {
      if (!dictionariesReady) return nullptr;
      if (sheetSettings->bookDictionaryPath.empty()) return tr(STR_DICT_USE_GLOBAL);
      for (size_t i = 1; i < sheetSettings->dictionaryPaths.size(); ++i) {
        if (sheetSettings->dictionaryPaths[i] == sheetSettings->bookDictionaryPath)
          return sheetSettings->dictionaryLabels[i].c_str();
      }
      return tr(STR_UNAVAILABLE);
    }
    case Row::Footnotes:
      if (!d || d->footnoteCount == 0) return nullptr;
      std::snprintf(buffer, size, "%u", d->footnoteCount);
      return buffer;
    case Row::AutoPageTurn:
      if (!d || !d->autoPageTurnActive) return tr(STR_OFF);
      std::snprintf(buffer, size, "%us", d->autoPageTurnSeconds);
      return buffer;
    case Row::Advanced:
      return tr(STR_PICCO_SHEET_ADVANCED_HINT);
#if CROSSINK_SCALABLE_FONTS
    case Row::TtfHinting:
    case Row::TtfRaster:
    case Row::TtfInterpreter:
    case Row::TtfWeight:
    case Row::TtfSlant:
      return TtfRenderOptionsActivity::rowValue(ttfRow(row), sheetSettings->ttfRenderProfile);
#endif
    default:
      return nullptr;
  }
}

bool FrontlightPanelActivity::sheetRowOn(const Row row) const {
  if (row == Row::DarkMode) return SETTINGS.screenInverted != 0;
  if (row == Row::TrackStats) return context.readerSheet && context.readerSheet->bookStats;
  if (!sheetSettings) return false;
  const ReaderSettingsDraft& draft = sheetSettings->draft;
  switch (row) {
    case Row::Hyphenation:
      return draft.hyphenationEnabled;
    case Row::ForceIndents:
      return draft.forceParagraphIndents;
    case Row::ExtraSpacing:
      return draft.extraParagraphSpacing;
    case Row::Focus:
      return draft.focusReadingEnabled;
    case Row::GuideDots:
      return draft.guideReadingEnabled;
    case Row::TextAa:
      return draft.textAntiAliasing;
    case Row::EmbeddedStyle:
      return draft.embeddedStyle;
    case Row::PublisherPages:
      return draft.publisherPageNumbers;
#if CROSSINK_SCALABLE_FONTS
    case Row::TtfStemDarkening:
      return sheetSettings->ttfRenderProfile.stemDarkening;
#endif
    default:
      return false;
  }
}

bool FrontlightPanelActivity::sheetRowEnabled(const Row row) const {
  if (row == Row::DarkMode) return !slideSwitchOwns(CrossPointSettings::SlideSwitchAction::NightMode);
  return true;
}

// ---- Targets ---------------------------------------------------------------------------------

int FrontlightPanelActivity::sheetAddStop(const Box& box, const uint8_t radius, const bool adjustable) {
  if (sheetStopCount >= kMaxSheetStops) return -1;
  SheetStop& stop = sheetStops[sheetStopCount];
  stop.box = box;
  stop.radius = radius;
  stop.target = sheetTargetCount;
  stop.adjustable = adjustable;
  return sheetStopCount++;
}

int FrontlightPanelActivity::sheetAddTarget(const Box& box, const SheetHit hit, const uint8_t id, const int16_t value,
                                            const int stop) {
  if (sheetTargetCount >= kMaxSheetTargets) {
    LOG_ERR("SHEET", "Too many targets on pane %u", static_cast<unsigned>(sheetPane));
    return -1;
  }
  SheetTarget& target = sheetTargets[sheetTargetCount];
  target.box = box;
  target.hit = hit;
  target.id = id;
  target.value = value;
  target.stop = static_cast<int8_t>(stop);
  return sheetTargetCount++;
}

// ---- Drawing ---------------------------------------------------------------------------------

void FrontlightPanelActivity::sheetRender() {
  uint8_t* frame = renderer.getFrameBuffer();
  const size_t size = renderer.getBufferSize();
  if (sheetBackdrop && frame != nullptr) {
    if (!sheetBackdropSaved) {
      std::memcpy(sheetBackdrop.get(), frame, size);
      sheetBackdropSaved = true;
    } else {
      std::memcpy(frame, sheetBackdrop.get(), size);
    }
  }
  sheetBottom = sheetHeight();
  // Without the backdrop copy a shorter pane would leave the old sheet showing.
  if (!sheetBackdropSaved) sheetBottom = std::max(sheetBottom, sheetPaintedBottom);
  sheetDraw();
  sheetPaintedBottom = std::max(sheetPaintedBottom, sheetBottom);
  if (optionPopup.isActive()) optionPopup.render(renderer);
  renderer.displayBuffer();
}

void FrontlightPanelActivity::sheetDraw() {
  sheetTargetCount = 0;
  sheetStopCount = 0;
  D::sheet(renderer, renderer.getScreenWidth(), sheetBottom);
  sheetDrawHeader();
  switch (sheetPane) {
    case Pane::Main:
      sheetDrawMain();
      break;
    case Pane::GoTo:
      sheetDrawGoTo();
      break;
    case Pane::AutoPageTurn:
      sheetDrawAutoTurn();
      break;
    case Pane::Qr:
      sheetDrawQr();
      break;
    default:
      sheetDrawBlocks();
      break;
  }
  if (sheetRing && sheetFocus >= 0 && sheetFocus < sheetStopCount) {
    const SheetStop& stop = sheetStops[static_cast<size_t>(sheetFocus)];
    D::focusRing(renderer, stop.box, stop.radius);
    // Editing a value: a second ring.
    if (sheetSlider.editing)
      D::focusRing(renderer, Box{stop.box.x - 3, stop.box.y - 3, stop.box.width + 6, stop.box.height + 6},
                   stop.radius + 3);
  }
}

void FrontlightPanelActivity::sheetDrawHeader() {
  const Rect header{0, 0, renderer.getScreenWidth(), PiccoTokens::kHeaderHeight};
  HeaderChrome chrome;
  char age[16] = {};
  if (sheetPane == Pane::Main) {
    chrome.title = tr(STR_HOME);
    chrome.leading = HeaderLeading::House;
    if (sheetHasBookOrbit) {
      if (sheetOffline) {
        chrome.sync = HeaderSyncState::Offline;
      } else {
        chrome.sync = HeaderSyncState::Idle;
        const time_t now = time(nullptr);
        if (sheetSyncedAt != 0 && now >= static_cast<time_t>(sheetSyncedAt)) {
          PiccoHomeStops::formatSyncAge(static_cast<uint32_t>(now - sheetSyncedAt), tr(STR_PICCO_NOW), age,
                                        sizeof(age));
        }
        chrome.syncLabel = age;
      }
    }
  } else {
    chrome.title = paneTitle(sheetPane);
    chrome.leading = HeaderLeading::Back;
    if (sheetPane == Pane::Text) chrome.contextualLabel = tr(STR_PICCO_SHEET_MORE_TEXT);
  }
  GUI.drawHeaderChrome(renderer, header, chrome);
  const auto layout = PiccoTheme::headerLayout(renderer, header, chrome);
  sheetAddTarget(layout.leadingTouch, SheetHit::HeaderLeading, 0, 0,
                 sheetAddStop(layout.leadingTouch, PiccoTokens::kSmallRadius, false));
  if (!layout.sync.empty() && chrome.sync != HeaderSyncState::Hidden) {
    sheetAddTarget(layout.sync, SheetHit::HeaderSync, 0, 0,
                   sheetAddStop(layout.sync, PiccoTokens::kSmallRadius, false));
  }
  if (!layout.contextual.empty()) {
    sheetAddTarget(layout.contextual, SheetHit::HeaderContextual, 0, 0,
                   sheetAddStop(layout.contextual, PiccoTokens::kSmallRadius, false));
  }
}

void FrontlightPanelActivity::sheetDrawLight(const LightRow& row, const int index) {
  const bool brightnessRow = index == 0;
  D::icon(renderer, brightnessRow ? icon_picco_sheet_sun_28 : icon_picco_sheet_warmth_28, row.icon);
  if (brightnessRow && !slideSwitchOwnsLight()) {
    // The sun toggles the light; its target spans the row height left of the slider.
    const Box hit{0, row.row.y, row.track.x - PiccoTokens::kSliderKnob / 2, row.row.height};
    sheetAddTarget(hit, SheetHit::LightToggle, 0, 0, sheetAddStop(row.icon, PiccoTokens::kSmallRadius, false));
  }
  const uint8_t value = brightnessRow ? brightness : warmth;
  const bool off = brightnessRow && !Frontlight.isOn();
  D::slider(renderer, row.track, value * 10);
  char text[8];
  std::snprintf(text, sizeof(text), "%u%%", static_cast<unsigned>(value));
  D::text(renderer, SMALL_FONT_ID, row.value, off ? tr(STR_OFF) : text, EpdFontFamily::REGULAR, D::Align::Right);
  const Box sliderHit{row.track.x - PiccoTokens::kSliderKnob / 2, row.row.y, row.track.width + PiccoTokens::kSliderKnob,
                      row.row.height};
  sheetAddTarget(sliderHit, SheetHit::LightSlider, static_cast<uint8_t>(index), 0,
                 sheetAddStop(row.track, PiccoTokens::kSmallRadius, true));
}

void FrontlightPanelActivity::sheetDrawMain() {
  const int w = renderer.getScreenWidth();
  const bool book = sheetVariant != Variant::Outside;
  const auto& details = context.bookDetails;
  const Tiles tiles = sheetTiles();
  const MainLayout m = mainLayout(w, book, book && !details.author.empty(), Frontlight.hasColorTemperature(),
                                  tiles.count, sheetVariant == Variant::Epub);

  if (book) {
    D::sectionLabel(renderer, m.label, tr(STR_PICCO_SHEET_NOW_READING));
    const char* title = !details.title.empty() ? details.title.c_str() : context.bookTitle.c_str();
    D::text(renderer, LITERATA_15_FONT_ID, m.title, title);
    if (!details.author.empty()) D::text(renderer, UI_10_FONT_ID, m.author, details.author.c_str());

    char percent[8];
    std::snprintf(percent, sizeof(percent), "%d%%", std::clamp(details.progressPercent, 0, 100));
    const int percentW = renderer.getTextWidth(SMALL_FONT_ID, percent);
    D::text(renderer, SMALL_FONT_ID, m.goToLine, percent, EpdFontFamily::REGULAR, D::Align::Right);
    char line[160] = {};
    char pages[48] = {};
    if (details.chapterPageCount > 0) {
      std::snprintf(pages, sizeof(pages), tr(STR_PICCO_SHEET_PAGE_OF_FORMAT),
                    static_cast<unsigned>(std::min(details.chapterPage, details.chapterPageCount)),
                    static_cast<unsigned>(details.chapterPageCount));
    }
    if (!details.chapter.empty() && pages[0] != '\0') {
      std::snprintf(line, sizeof(line), "%s \xC2\xB7 %s", details.chapter.c_str(), pages);
    } else {
      std::snprintf(line, sizeof(line), "%s", !details.chapter.empty() ? details.chapter.c_str() : pages);
    }
    D::text(renderer, SMALL_FONT_ID,
            Box{m.goToLine.x, m.goToLine.y, m.goToLine.width - percentW - PiccoTokens::kGroupGap, m.goToLine.height},
            line);
    PiccoProgress::draw(renderer, toRect(m.progress), std::clamp(details.progressPercent, 0, 100) * 10);

    if (sheetVariant == Variant::Epub) {
      const char* goTo = tr(STR_PICCO_SHEET_GO_TO);
      const int chevron = icon_picco_chevron_right_22.w;
      const int labelW = renderer.getTextWidth(SMALL_FONT_ID, goTo, EpdFontFamily::BOLD);
      const int x = m.goToFoot.right() - chevron - labelW;
      D::text(renderer, SMALL_FONT_ID, Box{x, m.goToFoot.y, labelW + 1, m.goToFoot.height}, goTo, EpdFontFamily::BOLD);
      D::icon(renderer, icon_picco_chevron_right_22, Box{x + labelW, m.goToFoot.y, chevron, m.goToFoot.height});
      sheetAddTarget(m.goTo, SheetHit::GoToBlock, 0, 0, sheetAddStop(m.goTo, PiccoTokens::kSmallRadius, false));
    }
  }

  D::hairline(renderer, m.band.x, m.band.y, m.band.width);
  D::hairline(renderer, m.band.x, m.band.bottom() - PiccoTokens::kHairline, m.band.width);
  for (int i = 0; i < m.lightRows; ++i) sheetDrawLight(m.light[static_cast<size_t>(i)], i);

  const ReaderSheetDetails* d = context.readerSheet.get();
  for (int i = 0; i < m.tileCount; ++i) {
    const Box& box = m.tiles[static_cast<size_t>(i)];
    const Tile tile = tiles.items[static_cast<size_t>(i)];
    switch (tile) {
      case Tile::Text:
        D::tile(renderer, box, icon_picco_sheet_text_26, tr(STR_PICCO_SHEET_TEXT));
        break;
      case Tile::Bookmark:
        D::tile(renderer, box,
                d && d->pageBookmarked ? icon_picco_sheet_bookmark_filled_26 : icon_picco_sheet_bookmark_26,
                tr(STR_PICCO_SHEET_BOOKMARK));
        break;
      case Tile::Contents:
        D::tile(renderer, box, icon_picco_sheet_contents_26, tr(STR_PICCO_SHEET_CONTENTS));
        break;
      case Tile::More:
        D::tile(renderer, box, icon_picco_sheet_more_26, tr(STR_MORE));
        break;
    }
    sheetAddTarget(box, SheetHit::Tile, static_cast<uint8_t>(tile), 0,
                   sheetAddStop(box, PiccoTokens::kSmallRadius, false));
  }
}

// Page as QR (T_QrPage): the page's words as a code, which page, which book.
void FrontlightPanelActivity::sheetDrawQr() {
  const QrLayout q = qrLayout(renderer.getScreenWidth());
  renderer.drawRoundedRect(q.frame.x, q.frame.y, q.frame.width, q.frame.height, PiccoTokens::kHairline,
                           PiccoTokens::kSmallRadius, true);
  QrUtils::drawQrCode(renderer, toRect(q.code), sheetQrText);
  const ReaderSheetDetails* d = context.readerSheet.get();
  char caption[96];
  if (d && d->stablePageCount > 0) {
    std::snprintf(caption, sizeof(caption), tr(STR_PICCO_QR_SCAN_PAGE_FORMAT), static_cast<unsigned>(d->stablePage));
  } else {
    std::snprintf(caption, sizeof(caption), "%s", tr(STR_PICCO_QR_SCAN));
  }
  D::text(renderer, UI_10_FONT_ID, q.caption, caption, EpdFontFamily::REGULAR, D::Align::Center);
  const auto& details = context.bookDetails;
  const char* title = !details.title.empty() ? details.title.c_str() : context.bookTitle.c_str();
  char line[160];
  if (!details.chapter.empty()) {
    std::snprintf(line, sizeof(line), "%s \xC2\xB7 %s", title, details.chapter.c_str());
  } else {
    std::snprintf(line, sizeof(line), "%s", title);
  }
  D::text(renderer, SMALL_FONT_ID, q.detail, line, EpdFontFamily::REGULAR, D::Align::Center);
}

void FrontlightPanelActivity::sheetDrawBlocks() {
  const int w = renderer.getScreenWidth();
  const int tabs = tabCount(sheetPane, false);
  if (tabs > 1) {
    const TabBar bar = tabBar(w, PiccoTokens::kHeaderHeight + kTabTop);
    const char* first = "";
    const char* second = "";
    switch (sheetPane) {
      case Pane::Text:
        first = tr(STR_PICCO_SHEET_FONT_LAYOUT);
        second = tr(STR_PICCO_SHEET_PARAGRAPHS_STYLE);
        break;
      case Pane::More:
        first = tr(STR_PICCO_SHEET_NAVIGATE_SHARE);
        second = tr(STR_PICCO_SHEET_THIS_BOOK);
        break;
      default:
        first = tr(STR_BOOK);
        second = tr(STR_PICCO_SHEET_RESET);
        break;
    }
    D::tabBar(renderer, bar, first, second, sheetTab);
    for (int i = 0; i < 2; ++i) {
      const Box& cell = bar.cells[static_cast<size_t>(i)];
      sheetAddTarget(cell, SheetHit::Tab, static_cast<uint8_t>(i), 0,
                     sheetAddStop(cell, PiccoTokens::kTabCellRadius, false));
    }
  }

  const Blocks blocks = paneBlocks(sheetPane, sheetTab, sheetAvailability());
  const BlockLayout layout = layoutBlocks(blocks, w, paneContentTop(tabs), sheetPane != Pane::Text || sheetTab != 0);
  const ReaderSheetDetails* d = context.readerSheet.get();
  char value[48];
  for (int i = 0; i < blocks.count; ++i) {
    const Block& block = blocks.items[static_cast<size_t>(i)];
    const Box& box = layout.boxes[static_cast<size_t>(i)];
    const uint8_t id = static_cast<uint8_t>(block.row);
    if (layout.hairlineAbove[static_cast<size_t>(i)]) D::hairline(renderer, box.x, box.y, box.width);
    switch (block.kind) {
      case Kind::Heading:
        D::sectionLabel(renderer, box, sectionLabel(block.section));
        break;
      case Kind::Chip: {
        const ReaderSettingsDraft* draft = sheetSettings ? &sheetSettings->draft : nullptr;
        const bool sdFont = draft && draft->sdFontFamilyName[0] != '\0';
        const uint8_t family = draft ? draft->fontFamily : 0;
        if (block.row == Row::FontCurrent) {
          D::chip(renderer, box, "Aa", sdFont ? draft->sdFontFamilyName.data() : builtinFontName(family), true);
        } else if (block.row == Row::FontOther) {
          // The other built-in family: one tap away without scanning the SD card.
          D::chip(renderer, box, "Aa", builtinFontName(!sdFont && family == 0 ? 1 : 0), false);
        } else {
          D::chip(renderer, box, nullptr, tr(STR_PICCO_SHEET_MORE_FONTS), false);
        }
        sheetAddTarget(box, SheetHit::Chip, id, 0, sheetAddStop(box, PiccoTokens::kSmallRadius, false));
        break;
      }
      case Kind::Stepper: {
        D::rowLabel(renderer, box, sheetRowLabel(block.row), sheetRowHint(block.row), kStepperReserve);
        const Stepper s = stepper(box);
        D::stepper(renderer, s, sheetRowValue(block.row, value, sizeof(value)));
        const int stop = sheetAddStop(Box{s.minus.x, s.minus.y, s.plus.right() - s.minus.x, s.minus.height},
                                      PiccoTokens::kSmallRadius, true);
        const int16_t step = static_cast<int16_t>(stepSize(block.row));
        sheetAddTarget(s.minus, SheetHit::Minus, id, static_cast<int16_t>(-step), stop);
        sheetAddTarget(s.plus, SheetHit::Plus, id, step, stop);
        break;
      }
      case Kind::Segmented: {
        const bool alignment = block.row == Row::Alignment;
        const int count = alignment ? 5 : 4;
        const Segmented seg = segmented(box, count, alignment ? kAlignmentCellW : kOrientationCellW);
        D::rowLabel(renderer, box, sheetRowLabel(block.row), nullptr, seg.frame.width);
        const uint8_t* values = alignment ? kAlignmentValues : kOrientationValues;
        const uint8_t current =
            sheetSettings ? (alignment ? sheetSettings->draft.paragraphAlignment : sheetSettings->draft.orientation)
                          : 0;
        int selected = -1;
        for (int c = 0; c < count; ++c) {
          if (values[c] == current) selected = c;
        }
        D::segmented(renderer, seg, alignment ? kAlignmentIcons : kOrientationIcons, selected);
        const int stop = sheetAddStop(seg.frame, PiccoTokens::kTabRadius, true);
        for (int c = 0; c < count; ++c) {
          sheetAddTarget(seg.cells[static_cast<size_t>(c)], SheetHit::Segment, id, values[c], stop);
        }
        break;
      }
      case Kind::Switch: {
        D::rowLabel(renderer, box, sheetRowLabel(block.row), sheetRowHint(block.row), PiccoTokens::kSwitchW);
        const Box sw = rowSwitch(box);
        PiccoSwitch::draw(renderer, sw.x, sw.y, sheetRowOn(block.row));
        if (!sheetRowEnabled(block.row)) {
          renderer.fillRectDither(sw.x, sw.y, sw.width, sw.height, Color::LightGray);
          break;
        }
        sheetAddTarget(box, SheetHit::Switch, id, 0, sheetAddStop(box, PiccoTokens::kSmallRadius, false));
        break;
      }
      case Kind::Value: {
        const char* text = sheetRowValue(block.row, value, sizeof(value));
        const int chevron = icon_picco_chevron_right_22.w;
        const int textW = text ? std::min(box.width / 2, renderer.getTextWidth(SMALL_FONT_ID, text)) : 0;
        const int reserve = textW + chevron + 4;
        D::rowLabel(renderer, box, sheetRowLabel(block.row), sheetRowHint(block.row), reserve);
        if (text) {
          D::text(renderer, SMALL_FONT_ID, Box{box.right() - reserve, box.y, textW, box.height}, text,
                  EpdFontFamily::REGULAR, D::Align::Right);
        }
        D::icon(renderer, icon_picco_chevron_right_22, Box{box.right() - chevron, box.y, chevron, box.height});
        sheetAddTarget(box, SheetHit::Value, id, 0, sheetAddStop(box, PiccoTokens::kSmallRadius, false));
        break;
      }
      case Kind::Cell:
        D::cell(renderer, box, sheetRowLabel(block.row), sheetRowValue(block.row, value, sizeof(value)));
        sheetAddTarget(box, SheetHit::Cell, id, 0, sheetAddStop(box, PiccoTokens::kSmallRadius, false));
        break;
      case Kind::Button: {
        const char* label = sheetRowLabel(block.row);
        char counted[64];
        if (block.row == Row::DeleteBookmarks && d) {
          std::snprintf(counted, sizeof(counted), "%s (%u)", label, static_cast<unsigned>(d->bookmarkCount));
          label = counted;
        }
        D::rowLabel(renderer, box, label, sheetRowHint(block.row), kButtonW);
        const Box button = rowButton(box);
        D::frame(renderer, button);
        const bool reset =
            block.row == Row::ResetPace || block.row == Row::ResetBookSettings || block.row == Row::TtfReset;
        D::text(renderer, UI_10_FONT_ID, button, reset ? tr(STR_PICCO_SHEET_RESET) : tr(STR_DELETE),
                EpdFontFamily::REGULAR, D::Align::Center);
        sheetAddTarget(button, SheetHit::Button, id, 0, sheetAddStop(button, PiccoTokens::kSmallRadius, false));
        break;
      }
    }
  }
}

void FrontlightPanelActivity::sheetDrawGoTo() {
  const int w = renderer.getScreenWidth();
  const ReaderSheetDetails* d = context.readerSheet.get();
  const bool hasTabs = d && d->stablePageCount > 0;
  const GoToLayout g = goToLayout(w, hasTabs);
  if (hasTabs) {
    D::tabBar(renderer, g.tabs, tr(STR_PICCO_SHEET_PAGE), tr(STR_PICCO_SHEET_PERCENT), goToPercent ? 1 : 0);
    for (int i = 0; i < 2; ++i) {
      const Box& cell = g.tabs.cells[static_cast<size_t>(i)];
      sheetAddTarget(cell, SheetHit::Tab, static_cast<uint8_t>(i), 0,
                     sheetAddStop(cell, PiccoTokens::kTabCellRadius, false));
    }
  }

  const uint32_t target = goToTarget(goToEntry);
  char number[16];
  std::snprintf(number, sizeof(number), "%u", static_cast<unsigned>(goToEntry.value));
  const int numberW = renderer.getTextWidth(FIGURES_27_FONT_ID, number);
  D::text(renderer, FIGURES_27_FONT_ID, Box{g.number.x, g.number.y, numberW + 1, g.number.height}, number);
  if (goToPercent) {
    D::text(renderer, UI_12_FONT_ID, Box{g.number.x + numberW + 4, g.number.y, 60, g.number.height}, "%",
            EpdFontFamily::BOLD);
  }
  char detail[160];
  const auto& book = context.bookDetails;
  if (goToPercent) {
    char pages[48] = {};
    if (book.chapterPageCount > 0) {
      std::snprintf(pages, sizeof(pages), tr(STR_PICCO_SHEET_PAGE_OF_FORMAT),
                    static_cast<unsigned>(std::min(book.chapterPage, book.chapterPageCount)),
                    static_cast<unsigned>(book.chapterPageCount));
    }
    std::snprintf(detail, sizeof(detail), "%s%s%s", book.chapter.c_str(),
                  !book.chapter.empty() && pages[0] ? " \xC2\xB7 " : "", pages);
  } else {
    char of[32];
    std::snprintf(of, sizeof(of), tr(STR_PICCO_SHEET_OF_FORMAT), static_cast<unsigned>(goToEntry.maximum));
    std::snprintf(detail, sizeof(detail), "%s%s%s \xC2\xB7 %d%%", of, book.chapter.empty() ? "" : " \xC2\xB7 ",
                  book.chapter.c_str(), std::clamp(book.progressPercent, 0, 100));
  }
  D::text(renderer, SMALL_FONT_ID, g.detail, detail);

  D::slider(renderer, g.slider, goToPermille(goToEntry));
  sheetAddTarget(Box{g.slider.x, g.slider.y - 16, g.slider.width, g.slider.height + 32}, SheetHit::GoToSlider, 0, 0,
                 sheetAddStop(g.slider, PiccoTokens::kSmallRadius, true));

  static constexpr int16_t kSteps[4] = {-10, -1, 1, 10};
  static constexpr const char* kStepLabels[4] = {"-10", "-1", "+1", "+10"};
  for (int i = 0; i < 4; ++i) {
    const Box& box = g.steps[static_cast<size_t>(i)];
    D::frame(renderer, box);
    D::text(renderer, UI_10_FONT_ID, box, kStepLabels[i], EpdFontFamily::REGULAR, D::Align::Center);
    sheetAddTarget(box, SheetHit::GoToStep, 0, kSteps[i], sheetAddStop(box, PiccoTokens::kSmallRadius, false));
  }

  D::hairline(renderer, PiccoTokens::kGutter, g.rule, contentWidth(w));
  for (int i = 0; i < kGoToKeyRows * kGoToKeyCols; ++i) {
    const Box& box = g.keys[static_cast<size_t>(i)];
    D::frame(renderer, box);
    switch (keyAt(i)) {
      case Key::Backspace:
        D::icon(renderer, icon_picco_sheet_backspace_24, box);
        break;
      case Key::Clear:
        D::text(renderer, UI_10_FONT_ID, box, tr(STR_CLEAR_BUTTON), EpdFontFamily::REGULAR, D::Align::Center);
        break;
      case Key::Digit: {
        char digit[2] = {static_cast<char>('0' + keyDigit(i)), '\0'};
        D::text(renderer, UI_10_FONT_ID, box, digit, EpdFontFamily::REGULAR, D::Align::Center);
        break;
      }
    }
    sheetAddTarget(box, SheetHit::GoToKey, 0, static_cast<int16_t>(i),
                   sheetAddStop(box, PiccoTokens::kSmallRadius, false));
  }

  char pill[48];
  std::snprintf(pill, sizeof(pill),
                goToPercent ? tr(STR_PICCO_SHEET_GOTO_PERCENT_FORMAT) : tr(STR_PICCO_SHEET_GOTO_PAGE_FORMAT),
                static_cast<unsigned>(target));
  PiccoPillButton::draw(renderer, toRect(g.pill), pill);
  sheetAddTarget(g.pill, SheetHit::Pill, 0, 0, sheetAddStop(g.pill, PiccoTokens::kPillRadius, false));
}

void FrontlightPanelActivity::sheetDrawAutoTurn() {
  const AutoTurnLayout a = autoTurnLayout(renderer.getScreenWidth());
  char range[24];
  std::snprintf(range, sizeof(range), "%u\xE2\x80\x93%u s", static_cast<unsigned>(READER_AUTO_PAGE_TURN_MIN_SECONDS),
                static_cast<unsigned>(READER_AUTO_PAGE_TURN_MAX_SECONDS));
  D::rowLabel(renderer, a.row, tr(STR_PICCO_SHEET_TURN_EVERY), range, kStepperReserve);
  const Stepper s = stepper(a.row);
  char value[12];
  std::snprintf(value, sizeof(value), "%u s", static_cast<unsigned>(autoTurnSeconds));
  D::stepper(renderer, s, value);
  const int stop = sheetAddStop(Box{s.minus.x, s.minus.y, s.plus.right() - s.minus.x, s.minus.height},
                                PiccoTokens::kSmallRadius, true);
  const uint8_t id = static_cast<uint8_t>(Row::AutoPageTurn);
  sheetAddTarget(s.minus, SheetHit::Minus, id, -kAutoTurnStep, stop);
  sheetAddTarget(s.plus, SheetHit::Plus, id, kAutoTurnStep, stop);
  D::hairline(renderer, a.row.x, a.row.bottom() - PiccoTokens::kHairline, a.row.width);
  D::text(renderer, SMALL_FONT_ID, a.hint, tr(STR_PICCO_SHEET_AUTO_TURN_HINT));
  const ReaderSheetDetails* d = context.readerSheet.get();
  const bool running = d && d->autoPageTurnActive;
  PiccoPillButton::draw(renderer, toRect(a.pill), running ? tr(STR_PICCO_SHEET_STOP) : tr(STR_START));
  sheetAddTarget(a.pill, SheetHit::Pill, 0, 0, sheetAddStop(a.pill, PiccoTokens::kPillRadius, false));
}

// ---- Input -----------------------------------------------------------------------------------

void FrontlightPanelActivity::sheetSlide(const SheetTarget& target, const int x) {
  if (target.hit == SheetHit::GoToSlider) {
    const GoToLayout g =
        goToLayout(renderer.getScreenWidth(), context.readerSheet && context.readerSheet->stablePageCount);
    goToSetPermille(goToEntry, sliderPermilleAt(g.slider, x));
    requestUpdate();
    return;
  }
  const Box track{target.box.x + PiccoTokens::kSliderKnob / 2, target.box.y,
                  target.box.width - PiccoTokens::kSliderKnob, target.box.height};
  const uint8_t value = static_cast<uint8_t>((sliderPermilleAt(track, x) + 5) / 10);
  if (target.id == 0) {
    if (value == brightness) return;
    brightness = value;
    Frontlight.setBrightness(brightness);
    if (!slideSwitchOwnsLight()) {
      Frontlight.setOn(true);
      observedLightOn = true;
    }
  } else {
    if (value == warmth) return;
    warmth = value;
    Frontlight.setWarmth(warmth);
  }
  requestUpdate();
}

void FrontlightPanelActivity::sheetAdjust(const int stopIndex, const int delta) {
  if (stopIndex < 0 || stopIndex >= sheetStopCount) return;
  const SheetStop stop = sheetStops[static_cast<size_t>(stopIndex)];
  const SheetTarget first = sheetTargets[stop.target];
  switch (first.hit) {
    case SheetHit::LightSlider:
      if (first.id == 0) {
        adjustBrightness(delta * kLightButtonStep);
      } else {
        adjustWarmth(delta * kLightButtonStep);
      }
      return;
    case SheetHit::GoToSlider:
      goToStep(goToEntry, delta);
      requestUpdate();
      return;
    case SheetHit::Minus:
    case SheetHit::Plus:
      sheetActivate(sheetTargets[stop.target + (delta > 0 ? 1 : 0)]);
      return;
    case SheetHit::Segment: {
      // The segments of one control are consecutive targets.
      int count = 0;
      int selected = 0;
      const uint8_t current =
          sheetSettings ? (first.id == static_cast<uint8_t>(Row::Alignment) ? sheetSettings->draft.paragraphAlignment
                                                                            : sheetSettings->draft.orientation)
                        : 0;
      while (stop.target + count < sheetTargetCount && sheetTargets[stop.target + count].stop == stopIndex) {
        if (sheetTargets[stop.target + count].value == current) selected = count;
        ++count;
      }
      if (count == 0) return;
      sheetActivate(sheetTargets[stop.target + (selected + delta + count) % count]);
      return;
    }
    default:
      return;
  }
}

void FrontlightPanelActivity::sheetActivate(const SheetTarget& target) {
  const ReaderSheetDetails* d = context.readerSheet.get();
  const Row row = static_cast<Row>(target.id);
  switch (target.hit) {
    case SheetHit::HeaderLeading:
      if (sheetPane != Pane::Main) {
        sheetBack();
      } else if (context.sourceActivity && context.sourceActivity->isHomeActivity()) {
        close();
      } else {
        activityManager.goHome();
      }
      return;
    case SheetHit::HeaderSync:
      sheetFinish(ReaderSheetRequest{}, FrontlightPanelAction::BookOrbitSync);
      return;
    case SheetHit::HeaderContextual:
      sheetSetPane(Pane::TextMore);
      return;
    case SheetHit::Tab:
      if (sheetPane == Pane::GoTo) {
        sheetOpenGoTo(target.id == 1);
      } else {
        sheetTab = target.id;
        sheetFocus = sheetRing ? 0 : -1;
      }
      requestUpdate();
      return;
    case SheetHit::GoToBlock:
      sheetSetPane(Pane::GoTo);
      return;
    case SheetHit::LightToggle:
      toggleLight();
      return;
    case SheetHit::LightSlider:
    case SheetHit::GoToSlider:
      return;
    case SheetHit::Tile:
      switch (static_cast<Tile>(target.id)) {
        case Tile::Text:
          sheetSetPane(Pane::Text);
          return;
        case Tile::Bookmark:
          sheetRequest(static_cast<int16_t>(EpubReaderMenuAction::BOOKMARK_TOGGLE));
          return;
        case Tile::Contents:
          if (sheetVariant == Variant::Epub) {
            sheetRequest(static_cast<int16_t>(EpubReaderMenuAction::SELECT_CHAPTER));
          } else {
            sheetRequest(static_cast<int16_t>(XtcReaderMenuActivity::MenuAction::SELECT_CHAPTER));
          }
          return;
        case Tile::More:
          if (sheetVariant == Variant::Epub) {
            sheetSetPane(Pane::More);
          } else {
            ReaderSheetRequest request;
            request.openMenu = true;
            sheetFinish(request);
          }
          return;
      }
      return;
    case SheetHit::Chip:
      if (!sheetSettings) return;
      if (row == Row::FontOther) {
        const bool sdFont = sheetSettings->draft.sdFontFamilyName[0] != '\0';
        sheetSettings->selectBuiltinFontFamily(!sdFont && sheetSettings->draft.fontFamily == 0 ? 1 : 0);
        requestUpdate();
      } else if (row == Row::MoreFonts) {
        sheetShowFonts();
      }
      return;
    case SheetHit::Minus:
    case SheetHit::Plus:
      if (row == Row::AutoPageTurn) {
        autoTurnSeconds = static_cast<uint16_t>(std::clamp<int>(
            autoTurnSeconds + target.value, READER_AUTO_PAGE_TURN_MIN_SECONDS, READER_AUTO_PAGE_TURN_MAX_SECONDS));
        requestUpdate();
        return;
      }
      if (sheetSettings && sheetSettings->step(stepFor(row), target.value > 0 ? stepSize(row) : -stepSize(row)))
        requestUpdate();
      return;
    case SheetHit::Segment: {
      RowId id;
      if (sheetSettings && controllerRow(row, id) &&
          sheetSettings->setEnumValue(id, static_cast<uint8_t>(target.value)))
        requestUpdate();
      return;
    }
    case SheetHit::Switch:
    case SheetHit::Value:
    case SheetHit::Cell:
    case SheetHit::Button:
      sheetActivateRow(row);
      return;
    case SheetHit::GoToStep:
      goToStep(goToEntry, target.value);
      requestUpdate();
      return;
    case SheetHit::GoToKey:
      switch (keyAt(target.value)) {
        case Key::Backspace:
          goToBackspace(goToEntry);
          break;
        case Key::Clear:
          goToClear(goToEntry);
          break;
        case Key::Digit:
          goToDigit(goToEntry, keyDigit(target.value));
          break;
      }
      requestUpdate();
      return;
    case SheetHit::Pill:
      if (sheetPane == Pane::AutoPageTurn) {
        const bool running = d && d->autoPageTurnActive;
        sheetRequest(static_cast<int16_t>(EpubReaderMenuAction::AUTO_PAGE_TURN), running ? 0 : autoTurnSeconds);
      } else if (goToPercent) {
        sheetRequest(static_cast<int16_t>(EpubReaderMenuAction::GO_TO_PERCENT),
                     static_cast<int32_t>(goToTarget(goToEntry) * 100));
      } else {
        sheetRequest(static_cast<int16_t>(EpubReaderMenuAction::GO_TO_STABLE_PAGE), -1, goToTarget(goToEntry));
      }
      return;
  }
}

void FrontlightPanelActivity::sheetActivateRow(const Row row) {
  using A = EpubReaderMenuAction;
  const auto request = [this](const A action, const bool reopen = false) {
    sheetRequest(static_cast<int16_t>(action), -1, 0, reopen);
  };
  switch (row) {
    case Row::DarkMode:
      if (slideSwitchOwns(CrossPointSettings::SlideSwitchAction::NightMode)) return;
      SETTINGS.screenInverted = SETTINGS.screenInverted ? 0 : 1;
      display.setInverted(SETTINGS.screenInverted != 0);
      requestUpdate();
      return;
    case Row::TrackStats:
      request(A::TOGGLE_BOOK_STATS_TRACKING, true);
      return;
#if CROSSINK_SCALABLE_FONTS
    case Row::TtfStemDarkening:
      if (sheetSettings) sheetSettings->toggleTtfStemDarkening();
      requestUpdate();
      return;
    case Row::TtfReset:
      if (sheetSettings) sheetSettings->resetTtfRenderingProfile();
      requestUpdate();
      return;
#endif
    case Row::PublisherFonts:
    case Row::Images:
    case Row::RenderMode:
    case Row::DictionaryFont:
    case Row::DictionaryFontSize:
    case Row::TtfHinting:
    case Row::TtfRaster:
    case Row::TtfInterpreter:
    case Row::TtfWeight:
    case Row::TtfSlant:
      sheetShowRowOptions(row);
      return;
    case Row::FontRendering:
      sheetSetPane(Pane::FontRendering);
      return;
    case Row::BookDictionary:
      sheetShowDictionaries();
      return;
    case Row::AutoPageTurn:
      sheetSetPane(Pane::AutoPageTurn);
      return;
    case Row::Advanced:
      sheetSetPane(Pane::Advanced);
      return;
    case Row::Footnotes:
      request(A::FOOTNOTES);
      return;
    case Row::Bookmarks:
      request(A::VIEW_BOOKMARKS);
      return;
    case Row::Clippings:
      request(A::VIEW_CLIPPINGS);
      return;
    case Row::Lookup:
      request(A::LOOKUP);
      return;
    case Row::LookupHistory:
      request(A::LOOKUP_HISTORY);
      return;
    case Row::SaveClipping:
      request(A::SAVE_CLIPPING);
      return;
    case Row::NearbySync:
      request(A::NEARBY_POSITION_SYNC);
      return;
    case Row::SendBook:
      request(A::SEND_NEARBY_BOOK);
      return;
    case Row::Qr: {
      const ReaderSheetDetails* d = context.readerSheet.get();
      if (d && d->pageText) {
        sheetSetPane(Pane::Qr);
      } else {
        request(A::DISPLAY_QR);
      }
      return;
    }
    case Row::KOReaderSync:
      request(A::SYNC);
      return;
    case Row::Screenshot:
      request(A::SCREENSHOT);
      return;
    case Row::MarkFinished:
      request(A::TOGGLE_COMPLETED);
      return;
    case Row::BookStats:
      request(A::READING_STATS);
      return;
    case Row::StatusBar:
      request(A::STATUS_BAR_SETTINGS, true);
      return;
    case Row::Controls:
      request(A::CONTROLS_OPTIONS, true);
      return;
    case Row::ResetPace:
      request(A::RESET_READING_PACE);
      return;
    case Row::DeleteStats:
      request(A::DELETE_STATS);
      return;
    case Row::DeleteBookmarks:
      request(A::DELETE_BOOKMARKS);
      return;
    case Row::DeleteCache:
      request(A::DELETE_CACHE);
      return;
    case Row::ResetBookSettings:
      request(A::RESET_BOOK_READER_SETTINGS);
      return;
    default: {
      RowId id;
      if (sheetSettings && controllerRow(row, id) && sheetSettings->toggleSetting(id)) requestUpdate();
      return;
    }
  }
}

void FrontlightPanelActivity::sheetShowRowOptions(const Row row) {
  RowId id;
  if (!sheetSettings || !controllerRow(row, id)) return;
#if CROSSINK_SCALABLE_FONTS
  const bool ttf = row == Row::TtfHinting || row == Row::TtfRaster || row == Row::TtfInterpreter ||
                   row == Row::TtfWeight || row == Row::TtfSlant;
  const bool ready = ttf ? sheetSettings->showTtfRenderingOptions(id) : sheetSettings->showEnumOptions(renderer, id);
#else
  const bool ready = sheetSettings->showEnumOptions(renderer, id);
#endif
  if (!ready) {
    requestUpdate();
    return;
  }
  optionPopup.show(sheetSettings->enumOptionTitle, sheetSettings->enumOptionLabels,
                   sheetSettings->enumOptionSelectedIndex, [this](const int index) {
                     if (sheetSettings) sheetSettings->selectEnumOption(index);
                     requestUpdate();
                   });
  requestUpdate();
}

void FrontlightPanelActivity::sheetShowFonts() {
  sheetSettings->discoverFonts(renderer);
  if (sheetSettings->fontLabels.empty()) {
    requestUpdate();
    return;
  }
  optionPopup.show(StrId::STR_FONT_FAMILY, sheetSettings->fontLabels, sheetSettings->currentFontSelectionIndex(-1),
                   [this](const int index) {
                     if (sheetSettings) sheetSettings->previewFontFamily(index);
                     requestUpdate();
                   });
  requestUpdate();
}

void FrontlightPanelActivity::sheetShowDictionaries() {
  if (!sheetSettings) return;
  const ReaderSheetDetails* d = context.readerSheet.get();
  if (!dictionariesReady) {
    sheetSettings->discoverDictionaries(d ? d->epub.get() : nullptr);
    dictionariesReady = true;
  }
  int current = 0;
  for (size_t i = 1; i < sheetSettings->dictionaryPaths.size(); ++i) {
    if (sheetSettings->dictionaryPaths[i] == sheetSettings->bookDictionaryPath) current = static_cast<int>(i);
  }
  optionPopup.show(StrId::STR_BOOK_DICTIONARY, sheetSettings->dictionaryLabels, current, [this](const int index) {
    const ReaderSheetDetails* details = context.readerSheet.get();
    if (sheetSettings && details) sheetSettings->selectBookDictionary(details->epub.get(), index);
    requestUpdate();
  });
  requestUpdate();
}

void FrontlightPanelActivity::sheetLoop() {
  if (optionPopup.handleInput(mappedInput, [this] { requestUpdate(); })) return;

  if (observedLightOn != Frontlight.isOn()) {
    observedLightOn = Frontlight.isOn();
    requestUpdate();
  }

  int tx = 0;
  int ty = 0;
  if (sheetDragging) {
    if (mappedInput.isScreenTouchHeld(tx, ty)) {
      sheetSlide(sheetDragTarget, tx);
      return;
    }
    // The lift ends the drag; its tap or swipe belongs to the slider, not the sheet.
    sheetDragging = false;
    return;
  }

  // Copy what the last render published, then act without holding the render lock.
  const auto findTarget = [this](const int x, const int y, SheetTarget& out, const bool slidersOnly) {
    RenderLock lock(*this);
    for (int i = 0; i < sheetTargetCount; ++i) {
      const SheetTarget& t = sheetTargets[static_cast<size_t>(i)];
      const bool slider = t.hit == SheetHit::LightSlider || t.hit == SheetHit::GoToSlider;
      if (slider != slidersOnly) continue;
      if (t.box.contains(x, y)) {
        out = t;
        return true;
      }
    }
    return false;
  };

  if (mappedInput.wasScreenTouchDown(tx, ty)) {
    SheetTarget target;
    if (findTarget(tx, ty, target, true)) {
      sheetDragging = true;
      sheetDragTarget = target;
      sheetSlide(target, tx);
      return;
    }
  }

  MappedInputManager::SwipeDir direction = MappedInputManager::SwipeDir::None;
  int sx = 0;
  int sy = 0;
  int ex = 0;
  int ey = 0;
  if (mappedInput.wasSwipeWithPoints(direction, sx, sy, ex, ey)) {
    if (direction == MappedInputManager::SwipeDir::Up) {
      close();
      return;
    }
    const bool horizontal =
        direction == MappedInputManager::SwipeDir::Left || direction == MappedInputManager::SwipeDir::Right;
    const ReaderSheetDetails* d = context.readerSheet.get();
    if (horizontal && sy < sheetBottom && tabCount(sheetPane, d && d->stablePageCount > 0) > 1) {
      const uint8_t tab = direction == MappedInputManager::SwipeDir::Left ? 1 : 0;
      if (sheetPane == Pane::GoTo) {
        sheetOpenGoTo(tab == 1);
      } else {
        sheetTab = tab;
        sheetFocus = sheetRing ? 0 : -1;
      }
      requestUpdate();
    }
    return;
  }

  if (mappedInput.wasScreenTapped(tx, ty)) {
    if (ty >= sheetBottom) {
      close();
      return;
    }
    SheetTarget target;
    if (findTarget(tx, ty, target, false)) {
      if (sheetRing) {
        sheetRing = false;
        sheetSlider = {};
      }
      sheetActivate(target);
    }
    return;
  }

  // Rocker focus: the first press shows the ring, later presses move it or, while a value
  // is being edited, change that value (ReaderButtonSliderState).
  const auto press = [this](const ReaderButtonSliderInput input) {
    if (!sheetRing) {
      if (input == ReaderButtonSliderInput::Back) {
        sheetBack();
        return;
      }
      sheetRing = true;
      if (sheetFocus < 0) sheetFocus = 0;
      requestUpdate();
      return;
    }
    SheetStop stop{};
    SheetTarget first{};
    {
      RenderLock lock(*this);
      if (sheetFocus >= sheetStopCount) sheetFocus = sheetStopCount - 1;
      if (sheetFocus >= 0) {
        stop = sheetStops[static_cast<size_t>(sheetFocus)];
        first = sheetTargets[stop.target];
      }
    }
    const bool adjustable = sheetFocus >= 0 && stop.adjustable;
    if (!adjustable && (input == ReaderButtonSliderInput::Next || input == ReaderButtonSliderInput::Previous)) {
      sheetFocus = moveFocus(sheetFocus, input == ReaderButtonSliderInput::Next ? 1 : -1, sheetStopCount);
      requestUpdate();
      return;
    }
    if (!adjustable && input == ReaderButtonSliderInput::Confirm) {
      if (sheetFocus >= 0) sheetActivate(first);
      return;
    }
    if (!adjustable) {
      sheetBack();
      return;
    }
    const bool wasEditing = sheetSlider.editing;
    switch (readerButtonSliderInput(sheetSlider, input)) {
      case ReaderButtonSliderAction::Increase:
        sheetAdjust(sheetFocus, 1);
        return;
      case ReaderButtonSliderAction::Decrease:
        sheetAdjust(sheetFocus, -1);
        return;
      case ReaderButtonSliderAction::LeavePane:
        sheetBack();
        return;
      case ReaderButtonSliderAction::None:
        break;
    }
    if (!wasEditing && !sheetSlider.editing &&
        (input == ReaderButtonSliderInput::Next || input == ReaderButtonSliderInput::Previous)) {
      sheetFocus = moveFocus(sheetFocus, input == ReaderButtonSliderInput::Next ? 1 : -1, sheetStopCount);
    }
    requestUpdate();
  };

  using Button = MappedInputManager::Button;
  if (mappedInput.wasReleased(Button::Back)) {
    press(ReaderButtonSliderInput::Back);
  } else if (mappedInput.wasReleased(Button::Confirm)) {
    press(ReaderButtonSliderInput::Confirm);
  } else if (mappedInput.wasReleased(Button::Down) || mappedInput.wasReleased(Button::Right)) {
    press(ReaderButtonSliderInput::Next);
  } else if (mappedInput.wasReleased(Button::Up) || mappedInput.wasReleased(Button::Left)) {
    press(ReaderButtonSliderInput::Previous);
  }
}

#endif
