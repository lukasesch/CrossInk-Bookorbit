#pragma once
#include <I18n.h>

#include <functional>
#include <string>

#include "AppCapabilities.h"
#include "activities/Activity.h"
#include "components/OptionPopup.h"
#if CROSSINK_APP_PICCO_UI
#include "components/picco/PiccoDialog.h"
#endif

class ConfirmationActivity : public Activity {
 private:
  std::string popupTitle;
  OptionPopup confirmPopup;
  std::string customCancelLabel, customConfirmLabel;
  bool ignoreConfirmRelease = false;
  bool overrideDisabledReaderTouchscreen = false;
  StrId cancelLabel = StrId::STR_CANCEL;
  StrId confirmLabel = StrId::STR_CONFIRM;
#if CROSSINK_APP_PICCO_UI
  // Picco draws its own dialog (T_Dialog) instead of the option popup.
  bool picco = false;
  bool scrimApplied = false;
  std::string heading, body;
  PiccoDialog::Focus focus = PiccoDialog::Focus::None;
  PiccoDialogLayout::Geometry geometry{};
  const char* cancelText() const;
  const char* confirmText() const;
  void finishWith(bool confirmed);
  void loopPicco();
#endif

 public:
  ConfirmationActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, const std::string& heading,
                       const std::string& body, bool ignoreInitialConfirmRelease = false,
                       bool overrideDisabledReaderTouchscreen = false);

  // Replaces the Cancel / Confirm option labels. Call before the activity starts.
  void setOptionLabels(StrId cancel, StrId confirm) {
    cancelLabel = cancel;
    confirmLabel = confirm;
  }

  void setOptionLabels(const std::string& cancel, const std::string& confirm) {
    customCancelLabel = cancel;
    customConfirmLabel = confirm;
  }

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&& lock) override;
  bool allowPowerAsConfirmInReaderMode() const override { return true; }
};
