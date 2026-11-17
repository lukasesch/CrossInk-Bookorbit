// Picco status bar settings (T_StatusBarSettings, T_StatusBarSettings2). UI only: the rows are
// StatusBarSettingsActivity's slot model (left 1-3, centre, right 1-3, options) in sections,
// not the board's per-item switches, which the slot model cannot express.
#include "StatusBarSettingsActivity.h"

#if CROSSINK_APP_PICCO_UI

#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>
#include <string>

#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "components/TouchHeaderBackButton.h"
#include "components/UITheme.h"
#include "components/picco/PiccoSystemLists.h"
#include "components/picco/PiccoTokens.h"

namespace {
using PiccoSettingsRows::Row;
using PiccoSettingsRows::Trailing;

constexpr int kMaxRows = 24;
constexpr int kCardGap = 16;  // header hairline -> preview card (T_StatusBarSettings)
constexpr int kCardPadY = 12;
constexpr int kCardMinH = 64;
}  // namespace

int StatusBarSettingsActivity::piccoRows(int16_t* order, const char** sections, const int capacity) const {
  int rows = 0;
  for (int i = 0; i < visibleItemCount && rows + 2 <= capacity; ++i) {
    RowText text;
    describeRow(i, text);
    if (text.sectionHeading != nullptr) {
      sections[rows] = text.sectionHeading;
      order[rows++] = PiccoSystemLists::kSection;
    }
    sections[rows] = nullptr;
    order[rows++] = static_cast<int16_t>(i);
  }
  return rows;
}

void StatusBarSettingsActivity::piccoLoop() {
  int16_t order[kMaxRows];
  const char* sections[kMaxRows];
  const int rows = piccoRows(order, sections, kMaxRows);
  const char* title = displayContext       ? tr(STR_STATUS_BAR)
                      : view == View::Root ? tr(STR_STATUS_BARS)
                      : view == View::Top  ? tr(STR_TOP_STATUS_BAR)
                                           : tr(STR_BOTTOM_STATUS_BAR);
  const auto in = piccoList.input(
      mappedInput, renderer, buttonNavigator, title, rows, piccoRow,
      [&order, rows](const int row) { return row >= 0 && row < rows && order[row] >= 0; }, [this] { requestUpdate(); });
  switch (in.event) {
    case PiccoListView::Event::Back:
      goBack();
      if (view == View::Root) piccoRow = 0;
      return;
    case PiccoListView::Event::Activate:
    case PiccoListView::Event::Button: {
      const View before = view;
      selectedIndex = order[in.index];
      handleSelection();
      if (view != before) piccoRow = 0;
      requestUpdate();
      return;
    }
    case PiccoListView::Event::Step:
    case PiccoListView::Event::Action:
    case PiccoListView::Event::Dismiss:
    case PiccoListView::Event::None:
      return;
  }
}

void StatusBarSettingsActivity::renderPicco() {
  int16_t order[kMaxRows];
  const char* sections[kMaxRows];
  const int count = piccoRows(order, sections, kMaxRows);
  RowText texts[kMaxRows];
  Row rows[kMaxRows];
  for (int row = 0; row < count; ++row) {
    Row& r = rows[row];
    if (order[row] < 0) {
      r.section = true;
      r.label = sections[row];
      continue;
    }
    RowText& text = texts[row];
    describeRow(order[row], text);
    r.label = text.label;
    if (text.toggle) {
      r.trailing = Trailing::Switch;
      r.on = text.on;
    } else if (text.value != nullptr && std::string(text.value) == ">") {
      r.trailing = Trailing::Chevron;
    } else {
      r.value = text.value;
      r.trailing = Trailing::ValueChevron;
    }
  }
  if (piccoRow < 0 || piccoRow >= count || order[piccoRow] < 0) piccoRow = count > 1 && order[0] < 0 ? 1 : 0;

  const bool preview = view != View::Root && (displayContext || !SETTINGS.readerStatusBar(selectedPosition()).hidden);
  const auto position = selectedPosition();
  const int barHeight = displayContext
                            ? UITheme::getDisplayStatusBarTextHeight(renderer) + ReaderStatusBarConfig::TOP_TEXT_INSET
                            : UITheme::getReaderStatusBarHeight(position, renderer);
  const int cardH = std::max(kCardMinH, barHeight + 2 * kCardPadY);

  const char* title = displayContext       ? tr(STR_STATUS_BAR)
                      : view == View::Root ? tr(STR_STATUS_BARS)
                      : view == View::Top  ? tr(STR_TOP_STATUS_BAR)
                                           : tr(STR_BOTTOM_STATUS_BAR);
  PiccoListView::Frame frame;
  frame.title = title;
  frame.topReserve = preview ? kCardGap + cardH : 0;
  piccoList.draw(renderer, mappedInput, frame, rows, count, piccoRow);

  if (preview) {
    const Rect header = TouchHeaderBackButton::compactHeaderRect(renderer);
    const int cardY = header.y + header.height + kCardGap;
    const int cardW = renderer.getScreenWidth() - 2 * PiccoTokens::kGutter;
    // The bar keeps the reader's own side margins, so the preview sits on a hairline instead
    // of in the board's frame.
    renderer.fillRect(PiccoTokens::kGutter, cardY + cardH, cardW, PiccoTokens::kHairline, true);
    ReaderStatusBarContent content;
    content.outsideReader = displayContext;
    content.bookProgress = 75.12f;
    content.chapterProgress = 35.0f;
    content.chapterPage = 8;
    content.chapterPageCount = 32;
    content.stablePage = 120;
    content.stablePageCount = 540;
    content.bookTitle = tr(STR_EXAMPLE_BOOK);
    content.chapterTitle = tr(STR_EXAMPLE_CHAPTER);
    content.timeLeftBook = "3h 40m";
    content.timeLeftChapter = "1h 20m";
    content.previewClock = "12:34";
    content.previewOriginY = cardY + (cardH - barHeight) / 2;
    const auto displayConfig = SETTINGS.displayStatusBar.asReaderConfig();
    GUI.drawReaderStatusBar(renderer, position, content, displayContext ? &displayConfig : nullptr);
  }
  renderer.displayBuffer();
}

#endif
