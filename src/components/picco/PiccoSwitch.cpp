#include "PiccoSwitch.h"

#if CROSSINK_APP_PICCO_UI

#include <FreeInkUIGfxRenderer.h>
#include <GfxRenderer.h>

#include "PiccoLayout.h"

namespace PiccoSwitch {
namespace {
// Flash-resident: 2 x 288 bytes.
constexpr PiccoLayout::SwitchBits kOnBits = PiccoLayout::switchBits(true);
constexpr PiccoLayout::SwitchBits kOffBits = PiccoLayout::switchBits(false);

freeink::ui::BitmapRef makeRef(const PiccoLayout::SwitchBits& bits) {
  freeink::ui::BitmapRef ref;
  ref.data = bits.data();
  ref.width = PiccoTokens::kSwitchW;
  ref.height = PiccoTokens::kSwitchH;
  ref.format = freeink::ui::BitmapFormat::Mask1;
  return ref;
}

const freeink::ui::BitmapRef kOnRef = makeRef(kOnBits);
const freeink::ui::BitmapRef kOffRef = makeRef(kOffBits);
}  // namespace

const freeink::ui::BitmapRef& bitmap(const bool on) { return on ? kOnRef : kOffRef; }

void draw(const GfxRenderer& renderer, const int x, const int y, const bool on) {
  freeink::ui::GfxRendererTarget target(renderer);
  target.bitmap(
      freeink::ui::Rect{static_cast<int16_t>(x), static_cast<int16_t>(y), PiccoTokens::kSwitchW, PiccoTokens::kSwitchH},
      bitmap(on), freeink::ui::BitmapMode::Center);
}

void applyToListItem(freeink::ui::ListItem& item) {
  if (!item.toggle) return;
  item.valueIcon = &bitmap(item.toggleChecked);
  item.toggle = false;
  item.value = nullptr;
}

}  // namespace PiccoSwitch

#endif
