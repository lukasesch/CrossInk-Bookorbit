#include "PiccoOverlayDraw.h"

#if CROSSINK_APP_PICCO_UI

#include <GfxRenderer.h>

#include "fontIds.h"
#include "sheet/PiccoSheetDraw.h"

namespace PiccoOverlayDraw {

namespace {
constexpr int kMarkRadius = 5;
constexpr int kMarkOutset = 3;
}  // namespace

void popover(const GfxRenderer& renderer, const Box& frame) {
  using namespace PiccoTokens;
  renderer.fillRoundedRect(frame.x, frame.y, frame.width, frame.height, kDialogRadius, Color::White);
  renderer.drawRoundedRect(frame.x, frame.y, frame.width, frame.height, kHairline, kDialogRadius, true);
}

void button(const GfxRenderer& renderer, const Box& box, const char* label, const bool focused) {
  PiccoSheetDraw::frame(renderer, box, focused);
  const Box inner{box.x + PiccoTokens::kHeaderLeadingGap, box.y, box.width - 2 * PiccoTokens::kHeaderLeadingGap,
                  box.height};
  PiccoSheetDraw::text(renderer, SMALL_FONT_ID, inner, label, EpdFontFamily::REGULAR, PiccoSheetDraw::Align::Center);
}

void markFrame(const GfxRenderer& renderer, const Box& mark) {
  renderer.drawRoundedRect(mark.x - kMarkOutset, mark.y - kMarkOutset, mark.width + 2 * kMarkOutset,
                           mark.height + 2 * kMarkOutset, PiccoTokens::kSelectionStroke, kMarkRadius, true);
}

}  // namespace PiccoOverlayDraw

#endif
