#include "PiccoDialog.h"

#if CROSSINK_APP_PICCO_UI

#include <GfxRenderer.h>

#include <string>
#include <vector>

#include "fontIds.h"

namespace PiccoDialog {
namespace {
using PiccoLayout::Box;
constexpr int kTitleFont = UI_12_FONT_ID;
constexpr int kBodyFont = UI_10_FONT_ID;
constexpr int kTitleBodyGap = 12;
constexpr int kMaxTitleLines = 3;
constexpr int kMaxBodyLines = 8;
constexpr int kFocusOutset = 4;

void drawButton(const GfxRenderer& renderer, const Box& box, const char* label, const bool primary,
                const bool focused) {
  using namespace PiccoTokens;
  if (primary) {
    renderer.fillRoundedRect(box.x, box.y, box.width, box.height, kSmallRadius, Color::Black);
  } else {
    renderer.drawRoundedRect(box.x, box.y, box.width, box.height, kHairline, kSmallRadius, true);
  }
  const auto style = primary ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
  const std::string text = renderer.truncatedText(kBodyFont, label, box.width - 2 * kNeighbourGap, style);
  const int w = renderer.getTextWidth(kBodyFont, text.c_str(), style);
  const int y = box.y + (box.height - renderer.getLineHeight(kBodyFont)) / 2;
  renderer.drawText(kBodyFont, box.x + (box.width - w) / 2, y, text.c_str(), !primary, style);
  if (focused) {
    renderer.drawRoundedRect(box.x - kFocusOutset, box.y - kFocusOutset, box.width + 2 * kFocusOutset,
                             box.height + 2 * kFocusOutset, kSelectionStroke, kSmallRadius + kFocusOutset, true);
  }
}
}  // namespace

void scrim(const GfxRenderer& renderer) {
  uint8_t* buffer = renderer.getFrameBuffer();
  const int stride = renderer.getDisplayWidthBytes();
  if (buffer == nullptr || stride <= 0) return;
  const size_t size = renderer.getBufferSize();
  // A set bit is white: clearing every other pixel ORs a checkerboard of ink over the screen.
  for (size_t i = 0; i < size; ++i) {
    const bool oddRow = (i / stride) & 1;
    buffer[i] &= oddRow ? 0xAA : 0x55;
  }
}

PiccoDialogLayout::Geometry draw(const GfxRenderer& renderer, const char* title, const char* body,
                                 const char* cancelLabel, const char* confirmLabel, const Focus focus) {
  using namespace PiccoTokens;
  const Box screen{0, 0, renderer.getScreenWidth(), renderer.getScreenHeight()};
  const int textWidth = screen.width - 2 * (kDialogInset + kDialogPadding);
  const std::vector<std::string> titleLines =
      title != nullptr && title[0] != '\0'
          ? renderer.wrappedText(kTitleFont, title, textWidth, kMaxTitleLines, EpdFontFamily::BOLD)
          : std::vector<std::string>{};
  const std::vector<std::string> bodyLines = body != nullptr && body[0] != '\0'
                                                 ? renderer.wrappedText(kBodyFont, body, textWidth, kMaxBodyLines)
                                                 : std::vector<std::string>{};
  const int titleH = static_cast<int>(titleLines.size()) * renderer.getLineHeight(kTitleFont);
  const int bodyH = static_cast<int>(bodyLines.size()) * renderer.getLineHeight(kBodyFont);
  const int gap = titleH > 0 && bodyH > 0 ? kTitleBodyGap : 0;
  const auto g = PiccoDialogLayout::geometry(screen, titleH + gap + bodyH);

  renderer.fillRoundedRect(g.card.x, g.card.y, g.card.width, g.card.height, kDialogRadius, Color::White);
  renderer.drawRoundedRect(g.card.x, g.card.y, g.card.width, g.card.height, kHairline, kDialogRadius, true);
  const int x = g.card.x + kDialogPadding;
  int y = g.textTop;
  for (const auto& line : titleLines) {
    renderer.drawText(kTitleFont, x, y, line.c_str(), true, EpdFontFamily::BOLD);
    y += renderer.getLineHeight(kTitleFont);
  }
  y += gap;
  for (const auto& line : bodyLines) {
    renderer.drawText(kBodyFont, x, y, line.c_str());
    y += renderer.getLineHeight(kBodyFont);
  }
  drawButton(renderer, g.cancel, cancelLabel, false, focus == Focus::Cancel);
  drawButton(renderer, g.confirm, confirmLabel, true, focus == Focus::Confirm);
  return g;
}

}  // namespace PiccoDialog

#endif
