#include "EndOfBookOptions.h"

#include <FsHelpers.h>
#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>
#include <iterator>

#include "CrossPointSettings.h"
#include "ReaderUtils.h"
#include "components/UITheme.h"
#include "components/UIThemeTokens.h"
#include "components/UiAppHelpers.h"
#include "fontIds.h"
#include "util/ButtonNavigator.h"
#include "util/NextBookFinder.h"
#if CROSSINK_APP_PICCO_UI
#include <cctype>
#include <cstdio>

#include "BookStatsView.h"
#include "MappedInputManager.h"
#include "PiccoBookStatsLayout.h"
#include "components/TouchHeaderBackButton.h"
#include "components/picco/PiccoListUi.h"
#endif

namespace fui = freeink::ui;

namespace {
constexpr fui::ActionId ACTION_ROW = 1;

// Display name without the file extension, mirroring the file browser rows
std::string displayName(const std::string& filename) {
  const auto pos = filename.rfind('.');
  return filename.substr(0, pos);
}
}  // namespace

EndOfBookOptions::EndOfBookOptions(GfxRenderer& renderer)
    : renderer(renderer), uiTarget(makeUiTarget(renderer)), app(uiTarget, uiTarget.deviceContext()) {}

void EndOfBookOptions::loadOnce(const std::string& currentBookPath) {
  if (isLoaded.load(std::memory_order_acquire)) {
    return;
  }
  folder = FsHelpers::extractFolderPath(currentBookPath);
  names = NextBookFinder::findNextBooks(currentBookPath, MAX_SUGGESTIONS);
  selector.store(0, std::memory_order_relaxed);
  if (!names.empty()) {
    rowLabels.reserve(names.size() + 1);
    std::transform(names.begin(), names.end(), std::back_inserter(rowLabels), displayName);
    rowLabels.emplace_back(tr(STR_HOME));
    rowCount = static_cast<uint16_t>(rowLabels.size());
    for (uint16_t index = 0; index < rowCount; ++index) {
      rowItems[index].label = rowLabels[index].c_str();
      rowItems[index].actionValue = static_cast<int16_t>(index);
    }

    // One-time app setup on the render task, before the first render/route.
    applySharedUiTheme(app, uiTarget);
    app.on(ACTION_ROW, &EndOfBookOptions::onRowEvent, this);
    app.setScreen(&EndOfBookOptions::listScreen, this);
  }
  // Release-publish so the main task, which gates all access on isLoaded, never
  // observes a partially built list
  isLoaded.store(true, std::memory_order_release);
}

bool EndOfBookOptions::loaded() const { return isLoaded.load(std::memory_order_acquire); }

bool EndOfBookOptions::menuActive() const { return loaded() && (!names.empty() || UITheme::isPicco()); }

void EndOfBookOptions::setFinishedBook(const std::string& title, const std::string& author,
                                       const BookReadingStats& stats) {
  bookTitle = title;
  bookAuthor = author;
  bookFinished = stats.isCompleted;
  BookReadingStats::formatDuration(stats.totalReadingSeconds, stripValues[0], sizeof(stripValues[0]));
  snprintf(stripValues[1], sizeof(stripValues[1]), "%u", static_cast<unsigned>(stats.sessionCount));
  ReadingStatsDateTime today;
  const ReadingStatsDate end = stats.isCompleted && stats.finishedDate.isValid() ? stats.finishedDate
                               : getCurrentLocalReadingStatsDateTime(today)      ? today.date
                                                                                 : ReadingStatsDate{};
  if (stats.startDate.isValid() && end.isValid()) {
    const uint16_t days = readingSpanDaysElapsed(stats.startDate, end);
    snprintf(stripValues[2], sizeof(stripValues[2]), "%u %s", static_cast<unsigned>(days),
             days == 1 ? tr(STR_STATS_DAY) : tr(STR_STATS_DAYS));
  } else {
    snprintf(stripValues[2], sizeof(stripValues[2]), "-");
  }
}

std::string EndOfBookOptions::fullPath(const size_t index) const {
  if (index >= names.size()) {
    return {};
  }
  return folder == "/" ? "/" + names[index] : folder + "/" + names[index];
}

void EndOfBookOptions::onRowEvent(const fui::ActionEvent& event, void* user) {
  auto* self = static_cast<EndOfBookOptions*>(user);
  if (event.value < 0 || event.value > static_cast<int16_t>(self->names.size())) return;
  self->selector.store(event.value, std::memory_order_relaxed);
  // The tapped row leaves this screen (open book or home); a lingering flash
  // would gray an unrelated element on the next render.
  self->app.clearTapFlash();
}

EndOfBookOptions::Action EndOfBookOptions::handleMenuInput(const MappedInputManager& input, std::string* openPath) {
#if CROSSINK_APP_PICCO_UI
  if (UITheme::isPicco()) return handlePiccoInput(input, openPath);
#endif
  // Touch goes through the FreeInkApp: render() registered the row hit rects;
  // route the snapshot and consume the dispatched row from the returned event.
  if (uiReady.load(std::memory_order_acquire)) {
    const fui::InputSnapshot snap = touchSnapshotFrom(input);
    if (snap.touchPressed || snap.touchReleased) {
      const auto event = app.route(snap);
      if (event.action == ACTION_ROW && event.value >= 0 && event.value <= static_cast<int16_t>(names.size())) {
        const auto tappedRow = static_cast<size_t>(event.value);
        if (tappedRow < names.size()) {
          if (openPath) {
            *openPath = fullPath(tappedRow);
          }
          return Action::OpenBook;
        }
        return Action::GoHome;  // "Home" row tapped
      }
      if (app.invalidated()) {
        return Action::Redraw;
      }
    }
  }

  const int selectedIndex = selector.load(std::memory_order_relaxed);
  if (input.wasReleased(MappedInputManager::Button::Confirm)) {
    if (selectedIndex < static_cast<int>(names.size())) {
      if (openPath) {
        *openPath = fullPath(selectedIndex);
      }
      return Action::OpenBook;
    }
    return Action::GoHome;  // "Home" entry selected
  }

  // Short-press Back returns to the last page; a long press falls through to the
  // reader's own handler (file browser). Home is reached through the list's Home entry.
  if (input.wasReleased(MappedInputManager::Button::Back) && input.getHeldTime() < ReaderUtils::GO_HOME_MS) {
    return Action::LastPage;
  }

  // Suggestions are a menu: use spatial directions rather than reader page-turn actions.
  const int itemCount = static_cast<int>(names.size()) + 1;  // + "Home" entry
  if (input.wasReleased(MappedInputManager::Button::Up) || input.wasReleased(MappedInputManager::Button::Left)) {
    selector.store(ButtonNavigator::previousIndex(selectedIndex, itemCount),
                   std::memory_order_relaxed);  // wraps to the bottom
    return Action::Redraw;
  }
  if (input.wasReleased(MappedInputManager::Button::Down) || input.wasReleased(MappedInputManager::Button::Right)) {
    selector.store(ButtonNavigator::nextIndex(selectedIndex, itemCount),
                   std::memory_order_relaxed);  // wraps to the top
    return Action::Redraw;
  }
  return Action::None;
}

void EndOfBookOptions::listScreen(UiApp::ScreenType& screen, void* user) {
  static_cast<EndOfBookOptions*>(user)->buildListScreen(screen);
}

void EndOfBookOptions::buildListScreen(UiApp::ScreenType& screen) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  // Same layout math as render(): the list band starts under the title/subtitle it
  // draws, and stops above the button hints (the safe-area bottom edge).
  const Rect safe = UITheme::getInstance().getScreenSafeArea(renderer, true, false);
  const int titleY = safe.y + safe.height / 8;
  const int subtitleY = titleY + renderer.getLineHeight(UI_12_FONT_ID) + metrics.verticalSpacing;
  const int listTop = subtitleY + renderer.getLineHeight(UI_10_FONT_ID) + metrics.verticalSpacing * 2;
  setUiContentMargin(
      screen, renderer,
      fui::Insets{static_cast<int16_t>(listTop),
                  static_cast<int16_t>(renderer.getScreenWidth() - (safe.x + safe.width)),
                  static_cast<int16_t>(renderer.getScreenHeight() - (safe.y + safe.height) + metrics.verticalSpacing),
                  static_cast<int16_t>(safe.x)});

  fui::ListProps props;
  props.items = rowItems.data();
  props.count = rowCount;
  props.selectedIndex = static_cast<int16_t>(selector.load(std::memory_order_relaxed));
  props.action = ACTION_ROW;
  props.inputMask = fui::InputTouch;  // physical buttons stay in handleMenuInput()
  screen.list(props);
}

void EndOfBookOptions::render(GfxRenderer& renderer, const MappedInputManager& input) {
#if CROSSINK_APP_PICCO_UI
  if (UITheme::isPicco()) {
    renderPicco(renderer);
    return;
  }
#endif
  const auto& metrics = UITheme::getInstance().getMetrics();

  if (!menuActive()) {
    // No suggestions: the historical plain end screen. 3/8 of the screen height matches
    // the previous fixed position on the 480x800 panel and scales to other resolutions.
    renderer.drawCenteredText(UI_12_FONT_ID, renderer.getScreenHeight() * 3 / 8, tr(STR_END_OF_BOOK), true,
                              EpdFontFamily::BOLD);
    return;
  }

  // Suggestion menu: title, list (+ Home entry) and button hints. The hints are drawn at
  // the physical front buttons, which is a logical side/top edge in the rotated
  // orientations — lay out inside the safe area so nothing hides behind them. Vertical
  // positions derive from the safe-area height and font line heights so other panel
  // resolutions scale (review request on #2532).
  const Rect safe = UITheme::getInstance().getScreenSafeArea(renderer, true, false);
  const int titleY = safe.y + safe.height / 8;
  const int subtitleY = titleY + renderer.getLineHeight(UI_12_FONT_ID) + metrics.verticalSpacing;

  UITheme::drawCenteredText(renderer, safe, UI_12_FONT_ID, titleY, tr(STR_END_OF_BOOK), true, EpdFontFamily::BOLD);
  UITheme::drawCenteredText(renderer, safe, UI_10_FONT_ID, subtitleY, tr(STR_EOB_CONTINUE_WITH));

  // The list renders through the FreeInkApp so its rows register touch hit rects. The
  // orientation can have changed since construction (reader menu rotate), so re-derive
  // the device context before laying out.
  uiReady.store(false, std::memory_order_release);
  renderUiApp(app, uiTarget);
  uiReady.store(true, std::memory_order_release);

  const auto labels =
      input.mapLabels(input.withBackArrow(tr(STR_BACK)), tr(STR_OPEN), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
}

#if CROSSINK_APP_PICCO_UI
namespace {
namespace PB = PiccoBookStatsLayout;
using PiccoLayout::Box;

void drawLine(const GfxRenderer& renderer, const int font, const Box& box, const char* text,
              const EpdFontFamily::Style style = EpdFontFamily::REGULAR, const bool centered = false) {
  if (text == nullptr || text[0] == '\0' || box.width <= 0) return;
  const std::string visible = renderer.truncatedText(font, text, box.width, style);
  const int w = renderer.getTextWidth(font, visible.c_str(), style);
  renderer.drawText(font, centered ? box.x + (box.width - w) / 2 : box.x,
                    box.y + (box.height - renderer.getLineHeight(font)) / 2, visible.c_str(), true, style);
}

void drawCaps(const GfxRenderer& renderer, const Box& box, const char* text) {
  char caps[48];
  size_t i = 0;
  for (; text[i] != '\0' && i + 1 < sizeof(caps); ++i) {
    caps[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(text[i])));
  }
  caps[i] = '\0';
  drawLine(renderer, SMALL_FONT_ID, box, caps, EpdFontFamily::BOLD);
}

bool tapped(const MappedInputManager& input, const Rect& r) {
  return r.width > 0 && input.wasTapInRect(r.x, r.y, r.width, r.height);
}
}  // namespace

EndOfBookOptions::Action EndOfBookOptions::handlePiccoInput(const MappedInputManager& input, std::string* openPath) {
  const int rows = static_cast<int>(std::min<size_t>(names.size(), piccoRowCount));
  const auto openRow = [this, openPath](const int row) {
    if (openPath) *openPath = fullPath(static_cast<size_t>(row));
    return Action::OpenBook;
  };
  if (TouchHeaderBackButton::wasTapped(input, TouchHeaderBackButton::compactHeaderRect(renderer))) {
    return Action::GoHome;
  }
  for (int i = 0; i < rows; ++i) {
    if (tapped(input, piccoRows[static_cast<size_t>(i)])) return openRow(i);
  }
  if (tapped(input, piccoButtons[0])) return Action::OpenStats;
  if (tapped(input, piccoButtons[1])) return Action::OpenLibrary;

  // Rocker / buttons: the rows, then Book stats and Library.
  const int itemCount = rows + 2;
  const int selected = std::min(selector.load(std::memory_order_relaxed), itemCount - 1);
  if (input.wasReleased(MappedInputManager::Button::Confirm)) {
    if (selected < rows) return openRow(selected);
    return selected == rows ? Action::OpenStats : Action::OpenLibrary;
  }
  if (input.wasReleased(MappedInputManager::Button::Back) && input.getHeldTime() < ReaderUtils::GO_HOME_MS) {
    return Action::LastPage;
  }
  const bool previous =
      input.wasReleased(MappedInputManager::Button::Up) || input.wasReleased(MappedInputManager::Button::Left);
  const bool next =
      input.wasReleased(MappedInputManager::Button::Down) || input.wasReleased(MappedInputManager::Button::Right);
  if (!previous && !next) return Action::None;
  // The first press shows the focus frame where the selection already is.
  if (piccoFocus) {
    selector.store(previous ? ButtonNavigator::previousIndex(selected, itemCount)
                            : ButtonNavigator::nextIndex(selected, itemCount),
                   std::memory_order_relaxed);
  }
  piccoFocus = true;
  return Action::Redraw;
}

void EndOfBookOptions::renderPicco(GfxRenderer& renderer) {
  const Rect header = TouchHeaderBackButton::compactHeaderRect(renderer);
  TouchHeaderBackButton::draw(renderer, header, tr(STR_HOME), true);
  const Box screen{0, 0, renderer.getScreenWidth(), renderer.getScreenHeight()};
  const int x = screen.x + PiccoTokens::kGutter;
  const int w = screen.width - 2 * PiccoTokens::kGutter;
  const int smallH = renderer.getLineHeight(SMALL_FONT_ID);
  const int titleFont = renderer.filenameFontId(LITERATA_15_FONT_ID);
  const int titleH = renderer.getLineHeight(titleFont);
  const int bodyH = renderer.getLineHeight(UI_10_FONT_ID);

  int y = header.y + header.height + PB::kEobLabelTop;
  drawCaps(renderer, Box{x, y, w, smallH}, tr(STR_HOME_FINISHED));
  y += smallH + PB::kEobTitleGap;
  drawLine(renderer, titleFont, Box{x, y, w, titleH}, bookTitle.c_str());
  y += titleH;
  if (!bookAuthor.empty()) {
    y += 4;
    drawLine(renderer, UI_10_FONT_ID, Box{x, y, w, bodyH}, bookAuthor.c_str());
    y += bodyH;
  }
  y += PB::kEobStripGap;
  const Box strip{x, y, w, PB::kStripHeight};
  const char* values[3] = {stripValues[0], stripValues[1], stripValues[2]};
  const char* labels[3] = {tr(STR_STATS_TIME_LBL), tr(STR_STATS_SESSIONS_LBL), tr(STR_PICCO_TO_FINISH)};
  drawPiccoStatsStrip(renderer, strip, values, labels);
  y = strip.bottom();
  if (bookFinished) {
    y += PiccoTokens::kControlClearance;
    drawLine(renderer, SMALL_FONT_ID, Box{x, y, w, smallH}, tr(STR_MARKED_FINISHED));
    y += smallH;
  }

  piccoButtons[0] = PiccoListUi::toRect(PB::eobButton(screen, 0));
  piccoButtons[1] = PiccoListUi::toRect(PB::eobButton(screen, 1));
  const int rows = static_cast<int>(names.size());
  const int selected = selector.load(std::memory_order_relaxed);
  piccoRowCount = 0;
  if (rows > 0) {
    y += PiccoTokens::kSectionAbove;
    drawCaps(renderer, Box{x, y, w, smallH}, tr(STR_EOB_CONTINUE_WITH));
    const Box list{x, y + smallH + PiccoTokens::kSectionBelow, w, 0};
    const int limit = piccoButtons[0].y - PiccoTokens::kGroupGap;
    for (int i = 0; i < rows && i < static_cast<int>(MAX_SUGGESTIONS); ++i) {
      const Box row = PB::eobRow(list, i);
      if (row.bottom() > limit) break;
      renderer.fillRect(row.x, row.y, row.width, PiccoTokens::kHairline, true);
      const Box cover = PB::eobCover(row);
      renderer.drawRect(cover.x, cover.y, cover.width, cover.height, true);
      const std::string& name = rowLabels[static_cast<size_t>(i)];
      const char initial[2] = {name.empty() ? '?' : name[0], '\0'};
      drawLine(renderer, titleFont, cover, initial, EpdFontFamily::REGULAR, true);
      const int textX = cover.right() + PB::kEobCoverGap;
      const Box text{textX, row.y + (row.height - bodyH - smallH) / 2, row.right() - textX, bodyH};
      drawLine(renderer, LITERATA_10_FONT_ID, text, name.c_str());
      drawLine(renderer, SMALL_FONT_ID, Box{textX, text.bottom(), text.width, smallH}, tr(STR_PICCO_NEXT_IN_FOLDER));
      piccoRows[static_cast<size_t>(i)] = Rect{row.x, row.y, row.width, row.height};
      if (piccoFocus && selected == i) {
        PiccoListUi::drawFocusFrame(renderer, piccoRows[static_cast<size_t>(i)], PiccoTokens::kRowFrameRadius);
      }
      ++piccoRowCount;
    }
  }
  const char* buttonLabels[2] = {tr(STR_PICCO_BOOK_STATS), tr(STR_LIBRARY)};
  for (int i = 0; i < 2; ++i) {
    const bool focused = piccoFocus && selected == piccoRowCount + i;
    PiccoListUi::drawOutlineButton(renderer, piccoButtons[static_cast<size_t>(i)], buttonLabels[i], nullptr, focused,
                                   true);
  }
}
#endif
