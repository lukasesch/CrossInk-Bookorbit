#pragma once

#include "AppCapabilities.h"

#if CROSSINK_APP_PICCO_UI

#include <FreeInkUI.h>

class GfxRenderer;

// The one Picco switch: white 64 x 36 track, black knob right when on, outlined knob left
// when off. Drawn from a compile-time bitmap (PiccoLayout::switchBits) so hand-drawn
// screens and FreeInkUI list rows show identical pixels.
namespace PiccoSwitch {

const freeink::ui::BitmapRef& bitmap(bool on);
void draw(const GfxRenderer& renderer, int x, int y, bool on);
// Turns a ListItem toggle into a value-slot switch bitmap (the SDK toggle cannot draw the
// outlined knob). The row keeps its action; the caller still flips the value on tap.
void applyToListItem(freeink::ui::ListItem& item);

}  // namespace PiccoSwitch

#endif
