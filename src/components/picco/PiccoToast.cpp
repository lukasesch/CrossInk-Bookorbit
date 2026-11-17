#include "PiccoToast.h"

#if CROSSINK_APP_PICCO_UI

#include <GfxRenderer.h>
#include <Icon.h>

#include <algorithm>
#include <string>

#include "PiccoProgress.h"
#include "components/UiAppHelpers.h"
#include "fontIds.h"

namespace PiccoToast {
namespace {
constexpr int kBodyFont = UI_10_FONT_ID;
constexpr int kDetailFont = SMALL_FONT_ID;
constexpr int kContentGap = 12;
constexpr int kPadRight = 6;
constexpr int kActionPadX = 12;
}  // namespace

Rect rect(const GfxRenderer& renderer, const int headerBottom, const PiccoLayout::ToastKind kind) {
  const PiccoLayout::Box screen{0, 0, renderer.getScreenWidth(), renderer.getScreenHeight()};
  const auto b = PiccoLayout::toastRect(screen, headerBottom, kind);
  return Rect{b.x, b.y, b.width, b.height};
}

Rect draw(const GfxRenderer& renderer, const Rect& rect, const Content& content) {
  using PiccoLayout::ToastKind;
  const bool error = content.kind == ToastKind::Error || content.kind == ToastKind::ErrorTwoLine;
  const int stroke = error ? PiccoTokens::kSelectionStroke : PiccoTokens::kHairline;
  renderer.fillRoundedRect(rect.x, rect.y, rect.width, rect.height, PiccoTokens::kToastRadius, Color::White);
  renderer.drawRoundedRect(rect.x, rect.y, rect.width, rect.height, stroke, PiccoTokens::kToastRadius, true);

  int x = rect.x + PiccoTokens::kToastPadLeft;
  int right = rect.x + rect.width - kPadRight;
  const int centerY = rect.y + rect.height / 2;
  if (content.icon != nullptr) {
    drawLucideIcon(renderer, *content.icon, x, centerY - content.icon->h / 2);
    x += content.icon->w + kContentGap;
  }

  Rect action{};
  if (content.actionLabel != nullptr && content.actionLabel[0] != '\0') {
    const int labelW = renderer.getTextWidth(kBodyFont, content.actionLabel, EpdFontFamily::BOLD);
    const int actionW = labelW + 2 * kActionPadX;
    action = Rect{right - actionW, centerY - PiccoTokens::kActionHeight / 2, actionW, PiccoTokens::kActionHeight};
    renderer.drawText(kBodyFont, action.x + kActionPadX, centerY - renderer.getLineHeight(kBodyFont) / 2,
                      content.actionLabel, true, EpdFontFamily::BOLD);
    right = action.x - kContentGap;
  }

  const int textW = std::max(0, right - x);
  const char* title = content.title != nullptr ? content.title : "";
  const auto titleStyle = error ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
  const std::string visible = renderer.truncatedText(kBodyFont, title, textW, titleStyle);
  const int bodyLh = renderer.getLineHeight(kBodyFont);
  if (content.kind == ToastKind::Progress) {
    constexpr int kBarGap = 8;
    const int top = centerY - (bodyLh + kBarGap + PiccoTokens::kProgressFill) / 2;
    renderer.drawText(kBodyFont, x, top, visible.c_str(), true, titleStyle);
    PiccoProgress::draw(renderer, Rect{x, top + bodyLh + kBarGap, textW, PiccoTokens::kProgressFill},
                        content.progressPermille);
  } else if (error && content.detail != nullptr) {
    const int detailLh = renderer.getLineHeight(kDetailFont);
    const int maxLines = content.kind == ToastKind::ErrorTwoLine ? 2 : 1;
    const auto lines = renderer.wrappedText(kDetailFont, content.detail, std::max(1, textW), maxLines);
    const int blockH = bodyLh + detailLh * static_cast<int>(lines.size());
    int y = centerY - blockH / 2;
    renderer.drawText(kBodyFont, x, y, visible.c_str(), true, titleStyle);
    y += bodyLh;
    for (const auto& line : lines) {
      renderer.drawText(kDetailFont, x, y, line.c_str());
      y += detailLh;
    }
  } else {
    renderer.drawText(kBodyFont, x, centerY - bodyLh / 2, visible.c_str(), true, titleStyle);
  }
  return action;
}
}  // namespace PiccoToast

#endif
