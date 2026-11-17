#pragma once

#include "AppCapabilities.h"

#if CROSSINK_APP_PICCO_UI

#include <EpdFontFamily.h>

#include "PiccoSheetModel.h"

class GfxRenderer;
namespace freeink {
struct Icon;
}

// Drawing for the Picco reader sheet. Every function fills the boxes PiccoSheetModel
// computed, so hit tests and ink always agree.
namespace PiccoSheetDraw {

using PiccoLayout::Box;

enum class Align : uint8_t { Left, Center, Right };

// Text vertically centred in box, truncated to its width.
void text(const GfxRenderer& renderer, int fontId, const Box& box, const char* value,
          EpdFontFamily::Style style = EpdFontFamily::REGULAR, Align align = Align::Left);
// Section label: small bold capitals.
void sectionLabel(const GfxRenderer& renderer, const Box& box, const char* value);
void icon(const GfxRenderer& renderer, const freeink::Icon& icon, const Box& box);

// White sheet from the top edge to bottom with r22 lower corners, 1 px edge, handle and
// the 6 px dither strip under it.
void sheet(const GfxRenderer& renderer, int screenWidth, int bottom);
void hairline(const GfxRenderer& renderer, int x, int y, int width);
void focusRing(const GfxRenderer& renderer, const Box& element, int radius);

void slider(const GfxRenderer& renderer, const Box& track, int permille);
void tabBar(const GfxRenderer& renderer, const PiccoSheet::TabBar& bar, const char* first, const char* second,
            int selected);
void stepper(const GfxRenderer& renderer, const PiccoSheet::Stepper& stepper, const char* value);
void segmented(const GfxRenderer& renderer, const PiccoSheet::Segmented& control, const freeink::Icon* const* icons,
               int selected);
// Framed 60 px control (r14): chip, grid cell, keypad key, step key, action button.
void frame(const GfxRenderer& renderer, const Box& box, bool selected = false);
void cell(const GfxRenderer& renderer, const Box& box, const char* label, const char* value, bool enabled = true);
void tile(const GfxRenderer& renderer, const Box& box, const freeink::Icon& icon, const char* label);
void chip(const GfxRenderer& renderer, const Box& box, const char* sample, const char* label, bool selected);
// Label (and optional hint) on the left of a list row, leaving rightReserve px free.
void rowLabel(const GfxRenderer& renderer, const Box& row, const char* label, const char* hint, int rightReserve);

}  // namespace PiccoSheetDraw

#endif
