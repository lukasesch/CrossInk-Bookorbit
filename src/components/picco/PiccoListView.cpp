#include "PiccoListView.h"

#if CROSSINK_APP_PICCO_UI

#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>
#include <string>
#include <vector>

#include "AppVersion.h"
#include "MappedInputManager.h"
#include "PiccoPageIndicator.h"
#include "components/CompactHeader.h"
#include "components/TouchHeaderBackButton.h"
#include "components/TouchRegistry.h"
#include "fontIds.h"
#include "util/ButtonNavigator.h"

using PiccoSettingsRows::Row;
using PiccoSettingsRows::Trailing;

void PiccoListView::draw(GfxRenderer& renderer, const MappedInputManager& input, const Frame& frame, const Row* rows,
                         const int count, const int selected) {
  renderer.clearScreen();
  const Rect header = TouchHeaderBackButton::compactHeaderRect(renderer);
  if (input.hasTouchHardware()) {
    const int reserve =
        frame.action != nullptr ? header.x + header.width - TouchHeaderBackButton::trailingTouchRect(header).x : 0;
    TouchHeaderBackButton::drawCompact(renderer, frame.title, false, false,
                                       TouchHeaderBackButton::TITLE_VERTICAL_OFFSET, reserve);
    if (frame.action != nullptr) {
      TouchHeaderBackButton::drawTrailingIcon(renderer, header, *frame.action);
      TouchRegistry::getInstance().add(TouchHeaderBackButton::trailingTouchRect(header), kActionId,
                                       TouchRegistry::Item);
    }
  } else {
    CompactHeader::drawTitle(renderer, frame.title);
  }
  int top = header.y + header.height + frame.topReserve;
  int bottom = PiccoPageIndicator::listBottom(renderer);
  Rect band = PiccoPageIndicator::band(renderer);
  const int width = renderer.getScreenWidth() - 2 * PiccoTokens::kGutter;

  if (frame.note != nullptr && frame.note[0] != '\0') {
    // A help note sits between the header and the rows. Before a section (T_SettingsLight,
    // T_HyphenationPacks) it takes the first section's 20 px and the section keeps its 30 above.
    constexpr int kNoteLines = 3;
    const bool sectionNext = count > 0 && rows[0].section;
    const auto lines = renderer.wrappedText(SMALL_FONT_ID, frame.note, width, kNoteLines);
    int y = top + (sectionNext ? PiccoTokens::kSectionFirstAbove : PiccoTokens::kGroupGap);
    for (const auto& line : lines) {
      renderer.drawText(SMALL_FONT_ID, PiccoTokens::kGutter, y, line.c_str());
      y += renderer.getLineHeight(SMALL_FONT_ID);
    }
    top = sectionNext ? y + PiccoTokens::kSectionAbove - PiccoTokens::kSectionFirstAbove : y;
  }
  if (frame.versionFooter) {
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

  // Cold UI path, one redraw per input: the per-row layout lives only for this draw.
  std::vector<PiccoPagedList::Entry> entries(count);
  std::vector<PiccoPagedList::Slot> slots(count);
  for (int i = 0; i < count; ++i) {
    entries[i].section = rows[i].section;
    entries[i].height = PiccoSettingsRows::rowHeight(rows[i]);
  }
  const int pages =
      PiccoPagedList::paginate(entries.data(), count, top, bottom, PiccoSettingsRows::sectionTextHeight(renderer),
                               slots.data(), pageStarts, kMaxPages, /*continuedLabels=*/true);
  pageCount = pages;
  if ((focus || followSelection) && selected >= 0 && selected < count) {
    page = PiccoPagedList::pageOf(pageStarts, pages, selected);
  }
  page = std::clamp(page, 0, pages - 1);
  const int first = count > 0 ? pageStarts[page] : 0;
  const int end = PiccoPagedList::pageEnd(pageStarts, pages, page, count);
  if (PiccoPagedList::opensContinued(entries.data(), count, first)) {
    const int owner = PiccoPagedList::owningSection(entries.data(), first);
    PiccoSettingsRows::drawContinued(
        renderer, rows[owner].label,
        frame.continuedFormat != nullptr ? frame.continuedFormat : tr(STR_PICCO_CONTINUED_FORMAT),
        PiccoPagedList::continuedLabelY(top));
  }
  PiccoSettingsRows::drawPage(renderer, rows, slots.data(), first, end, focus ? selected : -1);
  for (int i = first; i < end; ++i) {
    if (rows[i].section) continue;
    // Later registrations win the hit test: controls sit over their row.
    TouchRegistry::getInstance().add(PiccoSettingsRows::rowRect(renderer, slots[i]), i, TouchRegistry::Item);
    if (rows[i].trailing == Trailing::Button) {
      TouchRegistry::getInstance().add(PiccoSettingsRows::buttonRect(renderer, rows[i], slots[i]), kButtonIdBase + i,
                                       TouchRegistry::Item);
    }
    if (rows[i].trailing == Trailing::Stepper) {
      const auto stepper = PiccoSettingsRows::stepperRects(renderer, slots[i]);
      const auto& m = stepper.minus;
      const auto& p = stepper.plus;
      TouchRegistry::getInstance().add(Rect{m.x, m.y, m.width, m.height}, kStepperIdBase + 2 * i, TouchRegistry::Item);
      TouchRegistry::getInstance().add(Rect{p.x, p.y, p.width, p.height}, kStepperIdBase + 2 * i + 1,
                                       TouchRegistry::Item);
    }
  }
  PiccoPageIndicator::draw(renderer, page + 1, pages, &band);
}

PiccoListView::Input PiccoListView::input(MappedInputManager& input, const GfxRenderer& renderer,
                                          ButtonNavigator& navigator, const char* title, const int count, int& selected,
                                          const std::function<bool(int)>& isRow, const std::function<void()>& redraw,
                                          const bool allowDismiss) {
  if (TouchHeaderBackButton::wasTapped(input, renderer, title) || input.wasPressed(MappedInputManager::Button::Back)) {
    return Input{Event::Back};
  }

  int tapped = -1;
  if (input.wasItemTapped(tapped)) {
    if (tapped == kActionId) return Input{Event::Action};
    if (tapped >= kStepperIdBase && tapped < kButtonIdBase) {
      const int row = (tapped - kStepperIdBase) / 2;
      if (!isRow(row)) return Input{};
      return Input{Event::Step, row, (tapped - kStepperIdBase) % 2 ? 1 : -1};
    }
    const bool button = tapped >= kButtonIdBase && tapped < kActionId;
    const int index = button ? tapped - kButtonIdBase : tapped;
    if (!isRow(index)) return Input{};
    selected = index;
    focus = false;
    return Input{button ? Event::Button : Event::Activate, index};
  }

  const auto swipe = input.wasSwipe();
  if (allowDismiss && swipe == MappedInputManager::SwipeDir::Up && input.wasBottomEdgeUpSwipe()) {
    return Input{Event::Dismiss};
  }
  const int pages = std::clamp(pageCount, 1, kMaxPages);
  const auto showPage = [&](const int next) {
    if (next < 0 || next >= pages || next == page) return;
    page = next;
    followSelection = false;
    if (focus) {
      const int end = PiccoPagedList::pageEnd(pageStarts, pages, next, count);
      for (int i = pageStarts[next]; i < end; ++i) {
        if (isRow(i)) {
          selected = i;
          break;
        }
      }
    }
    redraw();
  };
  if (swipe == MappedInputManager::SwipeDir::Up) {
    showPage(page + 1);
    return Input{};
  }
  if (swipe == MappedInputManager::SwipeDir::Down) {
    showPage(page - 1);
    return Input{};
  }

  // The rocker moves row by row; the draw follows the selection onto its page.
  const auto step = [&](const int direction) {
    if (!focus) {
      focus = true;
      int index = pageStarts[std::clamp(page, 0, kMaxPages - 1)];
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
    followSelection = true;
    redraw();
  };
  if (input.wasReleased(MappedInputManager::Button::Confirm)) {
    if (!focus) {
      step(1);
    } else if (isRow(selected)) {
      return Input{Event::Activate, selected};
    }
    return Input{};
  }
  navigator.onNext([&] { step(1); });
  navigator.onPrevious([&] { step(-1); });
  return Input{};
}

#endif
