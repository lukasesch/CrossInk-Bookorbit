#pragma once

#include "AppCapabilities.h"

#if CROSSINK_APP_PICCO_UI

#include "PiccoLayout.h"
#include "components/themes/BaseTheme.h"

namespace freeink {
struct Icon;
}

// Toast under the header, over the content (T_States): info / progress 76 tall with a
// 1 px frame, errors 84 / 104 with a 2 px frame; r14, icon + body text + optional action.
namespace PiccoToast {
struct Content {
  PiccoLayout::ToastKind kind = PiccoLayout::ToastKind::Info;
  const char* title = nullptr;
  const char* detail = nullptr;  // error second line
  const freeink::Icon* icon = nullptr;
  const char* actionLabel = nullptr;
  int progressPermille = -1;  // Progress kind only
};

Rect rect(const GfxRenderer& renderer, int headerBottom, PiccoLayout::ToastKind kind);
// Draws the toast; returns the action's touch rect (empty when there is none).
Rect draw(const GfxRenderer& renderer, const Rect& rect, const Content& content);
}  // namespace PiccoToast

#endif
