// Picco full-page picker for OptionPopup owners that opted in (T_OptionPicker, T_OptionPicker2):
// title in the header, the note under it, one check row per option, paged by swipe.
#include "OptionPopup.h"

#if CROSSINK_APP_PICCO_UI

#include <vector>

bool OptionPopup::handlePiccoInput(MappedInputManager& input, const std::function<void()>& requestUpdate) {
  const int count = static_cast<int>(labels().size());
  const auto in = piccoView.input(
      input, input.getRenderer(), buttonNavigator, title.c_str(), count, selectedIndex,
      [this, count](const int i) { return i >= 0 && i < count && !isDisabled(i) && i != inertOptionIndex; },
      requestUpdate);
  switch (in.event) {
    case PiccoListView::Event::Back:
      cancel(input, requestUpdate, true);
      break;
    case PiccoListView::Event::Activate:
    case PiccoListView::Event::Button:
      save(input, requestUpdate, false);
      break;
    case PiccoListView::Event::Step:
    case PiccoListView::Event::Action:
    case PiccoListView::Event::Dismiss:
    case PiccoListView::Event::None:
      break;
  }
  return true;
}

void OptionPopup::renderPicco(GfxRenderer& renderer, const MappedInputManager& input) const {
  const OptionLabels options = labels();
  const int count = static_cast<int>(options.size());
  // Cold path, one per redraw; labels borrow from the popup's own strings.
  std::vector<PiccoSettingsRows::Row> rows(count);
  for (int i = 0; i < count; ++i) {
    rows[i].label = options[i];
    rows[i].trailing = PiccoSettingsRows::Trailing::Check;
    rows[i].on = i == piccoCurrent;
    rows[i].bold = rows[i].on;
  }
  const Note& note = selectedNote();
  PiccoListView::Frame frame;
  frame.title = title.c_str();
  frame.note = note.body != nullptr && note.body[0] != '\0' ? note.body : note.boldLabel;
  piccoView.draw(renderer, input, frame, rows.data(), count, selectedIndex);
  renderer.displayBuffer();
}

#endif
