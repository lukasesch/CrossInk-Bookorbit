#include "ConfirmationActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include "components/UITheme.h"
#include "fontIds.h"

ConfirmationActivity::ConfirmationActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                           const std::string& heading, const std::string& body,
                                           bool ignoreInitialConfirmRelease, bool overrideDisabledReaderTouchscreen)
    : Activity("Confirmation", renderer, mappedInput),
      ignoreConfirmRelease(ignoreInitialConfirmRelease),
      overrideDisabledReaderTouchscreen(overrideDisabledReaderTouchscreen) {
  popupTitle.reserve(heading.size() + body.size() + 1);
  popupTitle = heading;
  if (!heading.empty() && !body.empty()) {
    popupTitle += ' ';
  }
  popupTitle += body;
#if CROSSINK_APP_PICCO_UI
  this->heading = heading;
  this->body = body;
#endif
}

void ConfirmationActivity::onEnter() {
  Activity::onEnter();
  if (overrideDisabledReaderTouchscreen) {
    mappedInput.setReaderTouchscreenOverride(true);
  }
#if CROSSINK_APP_PICCO_UI
  picco = UITheme::isPicco();
  if (picco) {
    scrimApplied = false;
    focus = PiccoDialog::Focus::None;
    requestUpdate(true);
    return;
  }
#endif

  const char* options[] = {customCancelLabel.empty() ? I18N.get(cancelLabel) : customCancelLabel.c_str(),
                           customConfirmLabel.empty() ? I18N.get(confirmLabel) : customConfirmLabel.c_str()};
  confirmPopup.show(popupTitle.c_str(), options, 2, 0, [this](int idx) {
    ActivityResult res;
    res.isCancelled = (idx != 1);
    setResult(std::move(res));
    finish();
  });
  confirmPopup.setPrimaryOptionIndex(1);

  requestUpdate(true);
}

void ConfirmationActivity::onExit() {
  if (overrideDisabledReaderTouchscreen) {
    mappedInput.setReaderTouchscreenOverride(false);
  }
  Activity::onExit();
}

void ConfirmationActivity::render(RenderLock&&) {
#if CROSSINK_APP_PICCO_UI
  if (picco) {
    // The screen below stays visible under the scrim; later renders only repaint the card.
    if (!scrimApplied) PiccoDialog::scrim(renderer);
    scrimApplied = true;
    geometry = PiccoDialog::draw(renderer, heading.c_str(), body.c_str(), cancelText(), confirmText(), focus);
    renderer.displayBuffer(HalDisplay::FAST_REFRESH);
    return;
  }
#endif
  if (confirmPopup.processRender(renderer, mappedInput)) return;
}

void ConfirmationActivity::loop() {
  if (ignoreConfirmRelease) {
    const bool confirmReleased = mappedInput.wasReleased(MappedInputManager::Button::Confirm);
    if (confirmReleased || !mappedInput.isPressed(MappedInputManager::Button::Confirm)) {
      ignoreConfirmRelease = false;
      return;
    }
  }

#if CROSSINK_APP_PICCO_UI
  if (picco) {
    loopPicco();
    return;
  }
#endif
  if (confirmPopup.handleInput(mappedInput, [this] { requestUpdate(); })) return;

  // Popup dismissed without a selection (Back button or tap outside): cancel.
  ActivityResult res;
  res.isCancelled = true;
  setResult(std::move(res));
  finish();
}

#if CROSSINK_APP_PICCO_UI
const char* ConfirmationActivity::cancelText() const {
  return customCancelLabel.empty() ? I18N.get(cancelLabel) : customCancelLabel.c_str();
}

const char* ConfirmationActivity::confirmText() const {
  return customConfirmLabel.empty() ? I18N.get(confirmLabel) : customConfirmLabel.c_str();
}

void ConfirmationActivity::finishWith(const bool confirmed) {
  ActivityResult res;
  res.isCancelled = !confirmed;
  setResult(std::move(res));
  finish();
}

void ConfirmationActivity::loopPicco() {
  using Focus = PiccoDialog::Focus;
  if (mappedInput.wasPressed(MappedInputManager::Button::Back)) {
    finishWith(false);
    return;
  }
  int x = 0;
  int y = 0;
  if (mappedInput.wasScreenTapped(x, y)) {
    if (geometry.confirm.contains(x, y)) {
      finishWith(true);
    } else if (geometry.cancel.contains(x, y) || !geometry.card.contains(x, y)) {
      finishWith(false);
    }
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (focus == Focus::None) {
      // The first press shows the focus on the safe choice, as the popup's first row did.
      focus = Focus::Cancel;
      requestUpdate();
    } else {
      finishWith(focus == Focus::Confirm);
    }
    return;
  }
  for (const auto button : {MappedInputManager::Button::Up, MappedInputManager::Button::Down,
                            MappedInputManager::Button::Left, MappedInputManager::Button::Right}) {
    if (mappedInput.wasReleased(button)) {
      focus = focus == Focus::Cancel ? Focus::Confirm : Focus::Cancel;
      requestUpdate();
      return;
    }
  }
}
#endif
