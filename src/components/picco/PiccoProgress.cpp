#include "PiccoProgress.h"

#if CROSSINK_APP_PICCO_UI

#include <GfxRenderer.h>

#include <algorithm>

#include "PiccoLayout.h"

namespace PiccoProgress {
void draw(const GfxRenderer& renderer, const Rect& rect, const int permille) {
  const auto g = PiccoLayout::progressGeometry(PiccoLayout::Box{rect.x, rect.y, rect.width, rect.height}, permille);
  if (!g.track.empty()) renderer.fillRect(g.track.x, g.track.y, g.track.width, g.track.height, true);
  if (!g.fill.empty()) {
    renderer.fillRoundedRect(g.fill.x, g.fill.y, g.fill.width, g.fill.height,
                             std::min(PiccoTokens::kProgressFillRadius, g.fill.width / 2), Color::Black);
  }
}
}  // namespace PiccoProgress

#endif
