// Picco look for the BookOrbit catalog (T_BookOrbit, T_BookDetail). Fetching, paging,
// downloads and the book actions stay in BookOrbitCatalogBrowserActivity.cpp; this file
// lays out and draws the Picco screens and maps their controls onto the same actions.
#include "BookOrbitCatalogBrowserActivity.h"

#if CROSSINK_APP_PICCO_UI

#include <GfxRenderer.h>
#include <I18n.h>
#include <Utf8.h>

#include <algorithm>
#include <string>

#include "MappedInputManager.h"
#include "components/OptionPopup.h"
#include "components/TouchHeaderBackButton.h"
#include "components/UITheme.h"
#include "components/UiAppHelpers.h"
#include "components/icons/libraryIcons.h"
#include "components/icons/listIcons.h"
#include "components/icons/piccoIcons.h"
#include "components/picco/PiccoLibraryLayout.h"
#include "components/picco/PiccoListUi.h"
#include "components/picco/PiccoPageIndicator.h"
#include "components/picco/PiccoPillButton.h"
#include "components/picco/PiccoProgress.h"
#include "fontIds.h"

namespace fui = freeink::ui;
using PiccoLayout::Box;
using PiccoListUi::toBox;
using PiccoListUi::toRect;

namespace {
constexpr fui::ActionId ACTION_ROW = 1;
constexpr fui::ActionId ACTION_CONTROL = 2;
constexpr fui::ActionId ACTION_PICCO = 3;
constexpr int16_t HEADER_CONTROL_SORT = 0;
// ACTION_PICCO values: a book row's Get button (its index in `entries`), then bases.
constexpr int16_t PICCO_TAB_BASE = 1000;
constexpr int16_t PICCO_SEARCH = 2000;
constexpr int16_t PICCO_DETAIL_BASE = 3000;
// Read-status tabs, in the order of the read-status choices (all, unread, reading, finished).
constexpr int TAB_COUNT = 4;
constexpr StrId TAB_LABELS[TAB_COUNT] = {StrId::STR_BOOKORBIT_STATUS_ALL, StrId::STR_BOOKORBIT_STATUS_UNREAD,
                                         StrId::STR_BOOKORBIT_STATUS_READING, StrId::STR_BOOKORBIT_STATUS_FINISHED};
// Detail buttons: the primary action, then Download again and Remove from device.
constexpr int DETAIL_PRIMARY = 0;
constexpr int DETAIL_REDOWNLOAD = 1;
constexpr int DETAIL_REMOVE = 2;

constexpr int kSmallFont = SMALL_FONT_ID;
constexpr int kBodyFont = UI_10_FONT_ID;
constexpr int kBookFont = LITERATA_10_FONT_ID;
constexpr int kBookTitleFont = LITERATA_15_FONT_ID;

Box screenBox(const GfxRenderer& renderer) { return Box{0, 0, renderer.getScreenWidth(), renderer.getScreenHeight()}; }

Rect headerRect(const GfxRenderer& renderer) {
  return UITheme::getHeaderRect(renderer, UITheme::getInstance().getMetrics().headerHeight);
}

int headerBottom(const GfxRenderer& renderer) {
  const Rect header = headerRect(renderer);
  return header.y + header.height;
}

template <typename Screen>
void hit(Screen& screen, const Box& box, const fui::ActionId action, const int value,
         const uint16_t mask = fui::InputTouch) {
  screen.frame().hit(fui::Rect{static_cast<int16_t>(box.x), static_cast<int16_t>(box.y),
                               static_cast<int16_t>(box.width), static_cast<int16_t>(box.height)},
                     action, static_cast<int16_t>(value), mask);
}

// The title's first character, upper-cased when it is ASCII (the cover placeholder).
std::string initialOf(const std::string& title) {
  if (title.empty()) return "?";
  const auto* bytes = reinterpret_cast<const unsigned char*>(title.c_str());
  const unsigned char lead = bytes[0];
  if (lead < 0x80) return std::string(1, static_cast<char>(std::toupper(lead)));
  const size_t length = lead >= 0xF0 ? 4 : lead >= 0xE0 ? 3 : 2;
  return title.substr(0, std::min(length, title.size()));
}

// Cover placeholder: frame, double rule at the top, the initial, a rule at the bottom.
void drawInitialCover(const GfxRenderer& renderer, const Box& cover, const std::string& title, const int pad) {
  renderer.drawRect(cover.x, cover.y, cover.width, cover.height, true);
  const int innerX = cover.x + pad;
  const int innerW = cover.width - 2 * pad;
  renderer.fillRect(innerX, cover.y + pad, innerW, 1, true);
  renderer.fillRect(innerX, cover.y + pad + 4, innerW, 1, true);
  renderer.fillRect(innerX, cover.bottom() - pad, innerW, 1, true);
  const std::string initial = initialOf(title);
  const int w = renderer.getTextWidth(kBookTitleFont, initial.c_str());
  renderer.drawText(kBookTitleFont, cover.x + (cover.width - w) / 2,
                    PiccoListUi::textTop(renderer, kBookTitleFont, cover.y + cover.height / 2), initial.c_str());
}
}  // namespace

void BookOrbitCatalogBrowserActivity::onPiccoEvent(const fui::ActionEvent& event, void* user) {
  auto* self = static_cast<BookOrbitCatalogBrowserActivity*>(user);
  self->app.clearTapFlash();
  const int value = event.value;
  if (value >= PICCO_DETAIL_BASE) {
    self->activatePiccoDetail(value - PICCO_DETAIL_BASE);
  } else if (value == PICCO_SEARCH) {
    self->launchSearch();
  } else if (value >= PICCO_TAB_BASE) {
    if (self->listOptionsAvailable()) self->applyListChoice(ListOption::Filter, value - PICCO_TAB_BASE);
  } else if (value >= 0 && value < static_cast<int>(self->entries.size())) {
    // Get: copies, since the download frees the listing.
    const Entry entry = self->entries[value];
    self->selectorIndex = self->windowStart + value;
    if (entry.type == EntryType::BOOK && !entry.onDevice) self->downloadBook(entry.bookId, entry.title);
  }
}

bool BookOrbitCatalogBrowserActivity::buildPiccoChrome(UiApp::ScreenType& screen) {
  const Box screenArea = screenBox(renderer);
  const int top = headerBottom(renderer);
  if (piccoDetailRow >= 0) {
    buildPiccoDetail(screen);
    return true;
  }
  if (mappedInput.hasTouchHardware() && listOptionsAvailable()) {
    hit(screen, toBox(TouchHeaderBackButton::trailingTouchRect(headerRect(renderer))), ACTION_CONTROL,
        HEADER_CONTROL_SORT);
  }

  if (navLevel == NavLevel::Books) {
    int listTop = top;
    if (listOptionsAvailable()) {
      // Read-status tabs (the board's New / Reading / All, plus Finished).
      const Box tabs = PiccoLibraryLayout::bandBelow(screenArea, top, PiccoTokens::kTabBarHeight);
      const char* labels[TAB_COUNT];
      for (int i = 0; i < TAB_COUNT; ++i) labels[i] = I18N.get(TAB_LABELS[i]);
      PiccoListUi::drawSegmented(renderer, toRect(tabs), labels, TAB_COUNT, readStatusChoice());
      for (int i = 0; i < TAB_COUNT; ++i) {
        hit(screen, PiccoLibraryLayout::segmentCell(tabs, TAB_COUNT, i), ACTION_PICCO, PICCO_TAB_BASE + i);
      }
      listTop = tabs.bottom();
    }
    buildPiccoBookRows(screen, listTop + PiccoLibraryLayout::kBandGap);
    return true;
  }

  int listTop = top + PiccoLibraryLayout::kBandGap;
  if (navLevel == NavLevel::Root) {
    // Search field: opens the keyboard (the root's Search row stays as well).
    const Box field = PiccoLibraryLayout::bandBelow(screenArea, top, PiccoTokens::kTextFieldHeight);
    renderer.drawRoundedRect(field.x, field.y, field.width, field.height, PiccoTokens::kHairline,
                             PiccoTokens::kSmallRadius, true);
    const auto& icon = icon_search_24;
    const int centerY = field.y + field.height / 2;
    drawLucideIcon(renderer, icon, field.x + PiccoTokens::kTextFieldPadX, centerY - icon.h / 2);
    const int textX = field.x + PiccoTokens::kTextFieldPadX + icon.w + PiccoTokens::kGroupGap;
    const std::string hint = renderer.truncatedText(kBodyFont, tr(STR_PICCO_SEARCH_BOOKORBIT),
                                                    field.right() - textX - PiccoTokens::kTextFieldPadX);
    renderer.drawText(kBodyFont, textX, PiccoListUi::textTop(renderer, kBodyFont, centerY), hint.c_str());
    hit(screen, field, ACTION_PICCO, PICCO_SEARCH);
    listTop = field.bottom() + PiccoLibraryLayout::kBandGap;
  }
  screen.setContentMarginFromScreen(
      fui::Insets{static_cast<int16_t>(listTop), 0,
                  static_cast<int16_t>(screenArea.height - PiccoLayout::listBottomLimit(screenArea)), 0});
  return false;
}

void BookOrbitCatalogBrowserActivity::buildPiccoBookRows(UiApp::ScreenType& screen, const int listTop) {
  const Box screenArea = screenBox(renderer);
  const Box list{PiccoTokens::kGutter, listTop, screenArea.width - 2 * PiccoTokens::kGutter,
                 PiccoLayout::maxOf(0, PiccoLayout::listBottomLimit(screenArea) - listTop)};
  visibleRows = std::max(1, PiccoLibraryLayout::rowsBetween(list.y, list.bottom(), PiccoLibraryLayout::kCatalogRow));
  // The loop owns topIndex; draw from the page holding it, within the rows in memory.
  const int top = std::max(windowStart, std::min(topIndex, std::max(windowStart, loadedEnd() - 1)));
  if (entries.empty()) return;
  const int titleH = renderer.getLineHeight(kBookFont);
  const int smallH = renderer.getLineHeight(kSmallFont);
  for (int slot = 0; slot < visibleRows; ++slot) {
    const int row = top + slot;
    const Entry* entry = entryAt(row);
    if (!entry) break;
    auto r = PiccoLibraryLayout::catalogRow(list, slot);
    const bool canGet = entry->type == EntryType::BOOK && !entry->onDevice;
    const auto& check = icon_picco_check_22;
    const char* state = tr(STR_BOOKORBIT_ON_DEVICE);
    const int stateW = check.w + 4 + renderer.getTextWidth(kSmallFont, state);
    // The on-device state is wider than the Get button: the text column gives way.
    if (!canGet)
      r.text.width = std::max(0, r.action.right() - stateW - PiccoLibraryLayout::kCatalogActionGap - r.text.x);
    if (slot > 0) renderer.fillRect(r.band.x, r.band.y, r.band.width, PiccoTokens::kHairline, true);
    drawInitialCover(renderer, r.cover, entry->title, 6);

    // Title (Literata), author, meta: the list section (series, day added) or "On device".
    const int blockH = titleH + 2 + 2 * smallH;
    int y = r.text.y + (r.text.height - blockH) / 2;
    const std::string title = renderer.truncatedText(kBookFont, entry->title.c_str(), r.text.width);
    renderer.drawText(kBookFont, r.text.x, y, title.c_str());
    y += titleH + 2;
    if (!entry->subtitle.empty()) {
      const std::string author = renderer.truncatedText(kSmallFont, entry->subtitle.c_str(), r.text.width);
      renderer.drawText(kSmallFont, r.text.x, y, author.c_str());
    }
    y += smallH;
    const char* meta = entry->type == EntryType::BOOK && !entry->sectionId.empty() ? entry->sectionId.c_str() : "";
    if (*meta != '\0') {
      const std::string visible = renderer.truncatedText(kSmallFont, meta, r.text.width);
      renderer.drawText(kSmallFont, r.text.x, y, visible.c_str());
    }

    // Trailing: Get for a server book, a check and "On device" for one already here.
    const int relative = row - windowStart;
    if (canGet) {
      PiccoListUi::drawOutlineButton(renderer, toRect(r.action), tr(STR_PICCO_GET), &icon_arrow_down_24, false, true);
      hit(screen, r.action, ACTION_PICCO, relative);
    } else {
      const int centerY = r.action.y + r.action.height / 2;
      const int x = r.action.right() - (stateW - check.w - 4);
      drawLucideIcon(renderer, check, x - 4 - check.w, centerY - check.h / 2);
      renderer.drawText(kSmallFont, x, PiccoListUi::textTop(renderer, kSmallFont, centerY), state);
    }
    const Box details{r.band.x, r.band.y, r.text.right() - r.band.x, r.band.height};
    hit(screen, details, ACTION_ROW, relative, fui::InputTouch | fui::InputLongPress);
    if (row == selectorIndex && piccoDetailRow < 0 && (piccoFocusShown || !mappedInput.hasTouchHardware())) {
      PiccoListUi::drawFocusFrame(renderer,
                                  toRect(Box{r.band.x, r.band.y + PiccoTokens::kRowFrameInsetY, r.band.width,
                                             r.band.height - 2 * PiccoTokens::kRowFrameInsetY}),
                                  PiccoTokens::kRowFrameRadius);
    }
  }
}

int BookOrbitCatalogBrowserActivity::piccoDetailButtonCount() const {
  const Entry* entry = entryAt(piccoDetailRow);
  return entry && entry->onDevice ? 3 : 1;
}

void BookOrbitCatalogBrowserActivity::buildPiccoDetail(UiApp::ScreenType& screen) {
  const Entry* entry = entryAt(piccoDetailRow);
  if (!entry) return;
  const auto d = PiccoLibraryLayout::bookDetail(screenBox(renderer), headerBottom(renderer));

  // Cover placeholder with the title in Literata and the author under it.
  renderer.drawRect(d.cover.x, d.cover.y, d.cover.width, d.cover.height, true);
  constexpr int kPadX = 10;
  constexpr int kPadY = 14;
  const int innerX = d.cover.x + kPadX;
  const int innerW = d.cover.width - 2 * kPadX;
  renderer.fillRect(innerX, d.cover.y + kPadY, innerW, 1, true);
  renderer.fillRect(innerX, d.cover.y + kPadY + 5, innerW, 1, true);
  const int bookH = renderer.getLineHeight(kBookFont);
  const auto coverLines = renderer.wrappedText(kBookFont, entry->title.c_str(), innerW, 4);
  int y = d.cover.y + (d.cover.height - static_cast<int>(coverLines.size()) * bookH) / 2;
  for (const auto& line : coverLines) {
    const int w = renderer.getTextWidth(kBookFont, line.c_str());
    renderer.drawText(kBookFont, innerX + (innerW - w) / 2, y, line.c_str());
    y += bookH;
  }
  if (!entry->subtitle.empty()) {
    const std::string author = renderer.truncatedText(kSmallFont, entry->subtitle.c_str(), innerW);
    const int w = renderer.getTextWidth(kSmallFont, author.c_str());
    renderer.drawText(kSmallFont, innerX + (innerW - w) / 2,
                      d.cover.bottom() - kPadY - renderer.getLineHeight(kSmallFont), author.c_str());
  }

  // Text column: title (Literata 15), author, meta.
  const int titleH = renderer.getLineHeight(kBookTitleFont);
  const auto titleLines = renderer.wrappedText(kBookTitleFont, entry->title.c_str(), d.text.width, 3);
  y = d.text.y;
  for (const auto& line : titleLines) {
    renderer.drawText(kBookTitleFont, d.text.x, y, line.c_str());
    y += titleH;
  }
  y += 4;
  if (!entry->subtitle.empty()) {
    const auto authorLines = renderer.wrappedText(kBodyFont, entry->subtitle.c_str(), d.text.width, 2);
    for (const auto& line : authorLines) {
      renderer.drawText(kBodyFont, d.text.x, y, line.c_str());
      y += renderer.getLineHeight(kBodyFont);
    }
  }
  y += 8;
  const char* meta = entry->onDevice ? tr(STR_BOOKORBIT_ON_DEVICE) : entry->sectionId.c_str();
  if (*meta != '\0') {
    const auto metaLines = renderer.wrappedText(kSmallFont, meta, d.text.width, 2);
    for (const auto& line : metaLines) {
      renderer.drawText(kSmallFont, d.text.x, y, line.c_str());
      y += renderer.getLineHeight(kSmallFont);
    }
  }

  renderer.fillRect(d.rule.x, d.rule.y, d.rule.width, d.rule.height, true);

  const char* primary = entry->onDevice ? tr(STR_OPEN) : tr(STR_DOWNLOAD);
  PiccoPillButton::draw(renderer, toRect(d.primary), primary);
  hit(screen, d.primary, ACTION_PICCO, PICCO_DETAIL_BASE + DETAIL_PRIMARY);
  if (piccoDetailFocus == DETAIL_PRIMARY) {
    PiccoListUi::drawFocusFrame(renderer, toRect(d.primary), PiccoTokens::kPillRadius);
  }
  if (!entry->onDevice) return;
  const char* labels[2] = {tr(STR_PICCO_DOWNLOAD_AGAIN), tr(STR_PICCO_REMOVE)};
  for (int i = 0; i < 2; ++i) {
    PiccoListUi::drawOutlineButton(renderer, toRect(d.secondary[i]), labels[i], nullptr, piccoDetailFocus == i + 1,
                                   true);
    hit(screen, d.secondary[i], ACTION_PICCO, PICCO_DETAIL_BASE + DETAIL_REDOWNLOAD + i);
  }
}

void BookOrbitCatalogBrowserActivity::openPiccoDetail(const int row) {
  const Entry* entry = entryAt(row);
  if (!entry || entry->type != EntryType::BOOK) return;
  piccoDetailRow = row;
  piccoDetailFocus = mappedInput.hasTouchHardware() ? -1 : DETAIL_PRIMARY;
  requestUpdate();
}

void BookOrbitCatalogBrowserActivity::handlePiccoDetailInput(const bool backRequested) {
  if (backRequested) {
    piccoDetailRow = -1;
    requestUpdate();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    activatePiccoDetail(piccoDetailFocus < 0 ? DETAIL_PRIMARY : piccoDetailFocus);
    return;
  }
  if (uiReady) {
    const fui::InputSnapshot snap = touchSnapshotFrom(mappedInput);
    if (snap.touchPressed || snap.touchReleased) {
      const auto event = app.route(snap);
      if (app.invalidated()) requestUpdate();
      if (event) return;
    }
  }
  const int count = piccoDetailButtonCount();
  const auto step = [this, count](const int direction) {
    const int from = piccoDetailFocus < 0 ? (direction > 0 ? count - 1 : 0) : piccoDetailFocus;
    piccoDetailFocus =
        direction > 0 ? ButtonNavigator::nextIndex(from, count) : ButtonNavigator::previousIndex(from, count);
    requestUpdate();
  };
  buttonNavigator.onNextRelease([&step] { step(1); });
  buttonNavigator.onPreviousRelease([&step] { step(-1); });
}

void BookOrbitCatalogBrowserActivity::activatePiccoDetail(const int button) {
  const Entry* current = entryAt(piccoDetailRow);
  if (!current) {
    piccoDetailRow = -1;
    requestUpdate();
    return;
  }
  // Copies: a download frees the listing this points into.
  const Entry entry = *current;
  const int row = piccoDetailRow;
  piccoDetailRow = -1;
  switch (button) {
    case DETAIL_PRIMARY:
      if (entry.onDevice) {
        openLocalBook(entry.path);
      } else {
        downloadBook(entry.bookId, entry.title);
      }
      return;
    case DETAIL_REDOWNLOAD:
      if (entry.onDevice) downloadBook(entry.bookId, entry.title, entry.path);
      return;
    case DETAIL_REMOVE:
      if (entry.onDevice) promptDeleteBook(row);
      return;
    default:
      requestUpdate();
      return;
  }
}

void BookOrbitCatalogBrowserActivity::renderPicco() {
  renderer.clearScreen();
  const Rect header = headerRect(renderer);
  const int pageHeight = renderer.getScreenHeight();
  const bool browsing = state == BrowserState::BROWSING;

  std::string title =
      navLevel == NavLevel::Root || piccoDetailRow >= 0 ? std::string(tr(STR_PICCO_BOOKORBIT)) : headerTitle();
  HeaderChrome chrome;
  chrome.leading = HeaderLeading::Back;
  chrome.title = title.c_str();
  if (browsing && piccoDetailRow < 0 && listOptionsAvailable()) {
    chrome.contextualIcon = listSortsDescending() ? &icon_arrow_down_wide_narrow_32 : &icon_arrow_up_narrow_wide_32;
  }
  GUI.drawHeaderChrome(renderer, header, chrome);

  const auto centeredLines = [this](const char* text, int y) {
    const int width = renderer.getScreenWidth() - 2 * PiccoTokens::kGutter;
    const int lineH = renderer.getLineHeight(kBodyFont);
    for (const auto& line : renderer.wrappedText(kBodyFont, text, width, 4)) {
      renderer.drawCenteredText(kBodyFont, y, line.c_str());
      y += lineH + 4;
    }
  };

  if (state == BrowserState::CHECK_WIFI || state == BrowserState::LOADING || state == BrowserState::WIFI_SELECTION) {
    centeredLines(statusMessage.c_str(), pageHeight / 2);
    renderer.displayBuffer();
    return;
  }
  if (state == BrowserState::ERROR) {
    renderer.drawCenteredText(kBodyFont, pageHeight / 2 - 50, tr(STR_ERROR_MSG), true, EpdFontFamily::BOLD);
    centeredLines(errorMessage.c_str(), pageHeight / 2 - 20);
    renderer.displayBuffer();
    return;
  }
  if (state == BrowserState::DOWNLOADING) {
    renderer.drawCenteredText(kBodyFont, pageHeight / 2 - 40, tr(STR_DOWNLOADING), true, EpdFontFamily::BOLD);
    renderer.drawCenteredText(kBodyFont, pageHeight / 2 - 10, statusMessage.c_str());
    if (downloadTotal > 0) {
      const int permille = static_cast<int>(static_cast<uint64_t>(downloadProgress) * 1000 / downloadTotal);
      PiccoProgress::draw(renderer,
                          Rect{PiccoTokens::kGutter, pageHeight / 2 + 20,
                               renderer.getScreenWidth() - 2 * PiccoTokens::kGutter, PiccoTokens::kProgressFill},
                          permille);
    }
    renderer.displayBuffer();
    return;
  }

  uiReady = false;
  renderPass = 0;
  renderFromTop = topIndex;
  renderFromSelection = selectorIndex;
  renderPassAgain = false;
  app.render();
  uiReady = true;

  if (piccoDetailRow < 0) {
    if (entries.empty()) {
      renderer.drawCenteredText(kBodyFont, pageHeight / 2, tr(STR_NO_ENTRIES));
    } else {
      const int total = navLevel == NavLevel::Books ? std::max(listTotal, loadedEnd()) : loadedEnd();
      PiccoPageIndicator::drawForList(renderer, topIndex, visibleRows, total);
    }
  }
  if (listOptions && listOptions->processRender(renderer, mappedInput)) return;
  renderer.displayBuffer();
}

#ifdef SIMULATOR
void BookOrbitCatalogBrowserActivity::simulatorShowListing(std::vector<Entry> rows, const int total, const bool books,
                                                           const bool detail) {
  RenderLock lock(*this);
  entries = std::move(rows);
  windowStart = 0;
  selectorIndex = 0;
  topIndex = 0;
  navLevel = books ? NavLevel::Books : NavLevel::Root;
  listIsLocal = false;
  listQuery = BookOrbitBookQuery{};
  listQuery.readStatus = "unread";
  listTitle = "New";
  listTotal = total;
  state = BrowserState::BROWSING;
  piccoDetailRow = detail ? 2 : -1;
  piccoDetailFocus = -1;
  requestUpdate();
}
#endif

#endif
