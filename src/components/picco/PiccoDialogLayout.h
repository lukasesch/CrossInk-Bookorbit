#pragma once

#include "PiccoLayout.h"
#include "PiccoTokens.h"

// Confirmation dialog (T_Dialog): DarkGray scrim over the screen below, white card gutter to
// gutter, r22, 1 px frame, 24 padding, title + body, an equal Cancel / primary button pair.
namespace PiccoDialogLayout {

struct Geometry {
  PiccoLayout::Box card;
  PiccoLayout::Box cancel;
  PiccoLayout::Box confirm;
  int textTop = 0;  // title's first line
};

// contentHeight = title + gap + body; the card centres vertically on the screen.
constexpr Geometry geometry(const PiccoLayout::Box& screen, const int contentHeight) {
  using namespace PiccoTokens;
  Geometry g{};
  const int width = PiccoLayout::maxOf(0, screen.width - 2 * kDialogInset);
  const int height = 2 * kDialogPadding + contentHeight + kDialogPadding + kDialogButtonHeight;
  const int y = screen.y + PiccoLayout::maxOf(0, (screen.height - height) / 2);
  g.card = PiccoLayout::Box{screen.x + kDialogInset, y, width, height};
  g.textTop = y + kDialogPadding;
  const int inner = PiccoLayout::maxOf(0, width - 2 * kDialogPadding);
  const int buttonW = (inner - kDialogButtonGap) / 2;
  const int buttonY = g.card.bottom() - kDialogPadding - kDialogButtonHeight;
  g.cancel = PiccoLayout::Box{g.card.x + kDialogPadding, buttonY, buttonW, kDialogButtonHeight};
  g.confirm = PiccoLayout::Box{g.card.right() - kDialogPadding - buttonW, buttonY, buttonW, kDialogButtonHeight};
  return g;
}

}  // namespace PiccoDialogLayout
