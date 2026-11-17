#include "PiccoPillButton.h"

#if CROSSINK_APP_PICCO_UI

#include <GfxRenderer.h>
#include <Icon.h>

#include "PiccoLayout.h"
#include "components/UiAppHelpers.h"
#include "fontIds.h"

namespace PiccoPillButton {
namespace {
constexpr int kFont = UI_10_FONT_ID;
}

Rect rect(const GfxRenderer& renderer, const int x, const int centerY, const char* label, const freeink::Icon* icon) {
  const int labelW = renderer.getTextWidth(kFont, label != nullptr ? label : "", EpdFontFamily::BOLD);
  const int width = PiccoLayout::pillWidth(labelW, icon != nullptr ? icon->w : 0);
  return Rect{x, centerY - PiccoTokens::kPillHeight / 2, width, PiccoTokens::kPillHeight};
}

void draw(const GfxRenderer& renderer, const Rect& rect, const char* label, const freeink::Icon* icon) {
  const char* text = label != nullptr ? label : "";
  renderer.fillRoundedRect(rect.x, rect.y, rect.width, rect.height, PiccoTokens::kPillRadius, Color::Black);
  const int labelW = renderer.getTextWidth(kFont, text, EpdFontFamily::BOLD);
  const int contentW = labelW + (icon != nullptr ? icon->w + PiccoTokens::kHeaderLeadingGap : 0);
  const int textX = rect.x + (rect.width - contentW) / 2;
  const int textY = rect.y + (rect.height - renderer.getLineHeight(kFont)) / 2;
  renderer.drawText(kFont, textX, textY, text, false, EpdFontFamily::BOLD);
  if (icon != nullptr) {
    drawLucideIcon(renderer, *icon, textX + labelW + PiccoTokens::kHeaderLeadingGap,
                   rect.y + (rect.height - icon->h) / 2, false);
  }
}
}  // namespace PiccoPillButton

#endif
