---
title: BOOX Picco
nav_order: 15
---

# BOOX Picco

Picco has page-turn buttons, a touchscreen, a slide switch, and a frontlight
with adjustable brightness and warmth.

## Slide switch

Open **Settings > Controls > Slide Switch** and choose **Frontlight**,
**Dark Mode**, **Touch Reader Controls**, or **Tilt Page Turn**.
The switch position is the state of that feature: on means on, off means off.

The position is applied when the switch moves, when the assignment changes, and
at every start and wake, so the feature always matches the switch. While the
switch owns a feature, the other controls for it are hidden or do nothing. See
[Controls](./controls.md#slide-switch).

## Touch and frontlight

- Swipe down from the top edge to open the frontlight panel.
- While reading, swipe down anywhere to open that panel, or swipe up to open
  the Reader Menu. Use the panel's Home button to leave the book.
- Outside a book, swipe up from the bottom edge to return Home.
- Adjust brightness and warmth in the frontlight panel. Its icons follow any
  changes made with the slide switch.

With the **Picco** theme (Settings > Display > Theme) the panel becomes the
reader sheet, and the reader has no separate menu any more:

| Gesture or button | EPUB reader | TXT / XTC reader | Elsewhere |
| --- | --- | --- | --- |
| Swipe down | Reader sheet | Reader sheet | The screen's own (list paging) |
| Swipe up | Home | Home | The screen's own |
| Swipe down from the top edge | Reader sheet | Reader sheet | Sheet with header and light |
| Swipe up from the bottom edge | Home | Home | Home |
| Long-press | Word selection | Unchanged | The screen's own |
| Up / Down | Turn the page | Turn the page | Move the focus |
| Power | Power-button setting | Power-button setting | Power-button setting |
| Up + Down | Touch back on; with touch on, its mapped action (reader menu = sheet) | Same | Touch back on, else its mapped action |
| Back / Confirm (button-only devices) | Back setting / reader sheet | Same | Back / select |

The EPUB sheet shows the book, its progress (tap for **Go to**), brightness and
warmth, and four tiles: **Text** (font, size, spacing, margins, alignment,
orientation, paragraphs and style; **More text options** in its header),
**Bookmark** (adds or removes the page bookmark), **Contents** and **More**
(navigation, words, sync and share, this book, **Advanced**). TXT shows the
book and the light; XTC adds **Contents** and **More** (its menu list). The
sync button in the header runs BookOrbit sync. Inside the sheet, swipe up or
tap below it to close it, swipe left or right to switch a pane's two tabs, and
use the chevron or Back to go up one pane. With buttons, the first press shows
a focus ring; Confirm on a slider or stepper starts editing and Up / Down then
change the value. In landscape the panel and reader menu keep their classic
layout.

Two-finger gestures appear in Controls only when the touchscreen supports them.
See [Controls](./controls.md) for pinch, rotation, and swipe options.

## Sleep and charging

Sleeping turns the frontlight off. **Settings > Display > Frontlight > Restore
Light on Wake** controls whether it comes back on when you wake the reader.
Press Power to wake it. While plugged into USB, the sleep image stays visible;
it does not show live charging updates.

## Installation and updates

Use an installer that explicitly supports **BOOX Picco**, and select that exact
model. Picco needs an installation that preserves its factory recovery software.
Do not use firmware intended for an X4 Pro, erase the device, or select a full
factory-image installation.

CrossInk's Wi-Fi and SD-card firmware-update options are unavailable on Picco.
Use the supported Picco USB installation method for updates too.

To install this fork's Picco firmware, follow
[Installation > BOOX Picco](./installation.md#boox-picco).
