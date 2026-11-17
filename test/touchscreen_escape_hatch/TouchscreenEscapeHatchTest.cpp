#include <gtest/gtest.h>

#include "TouchscreenEscapeHatch.h"

namespace {

using TouchscreenEscapeHatch::Inputs;
using TouchscreenEscapeHatch::Route;
using TouchscreenEscapeHatch::routeFor;

// Picco in the reader with touch disabled.
constexpr Inputs piccoReaderTouchOff() {
  return {/*hasTouchHardware=*/true, /*touchDisabled=*/true,
          /*isReader=*/true,         /*globalLock=*/true,
          /*quickLocked=*/false,     /*modalOwnsInput=*/false};
}

}  // namespace

TEST(TouchscreenEscapeHatch, ReaderWithTouchOffEnablesTouch) {
  // Regression: the reader opened its menu drawer instead of turning touch on.
  EXPECT_EQ(routeFor(piccoReaderTouchOff()), Route::EnableTouch);
}

TEST(TouchscreenEscapeHatch, GlobalLockOutsideReaderEnablesTouch) {
  Inputs in = piccoReaderTouchOff();
  in.isReader = false;
  EXPECT_EQ(routeFor(in), Route::EnableTouch);
}

TEST(TouchscreenEscapeHatch, ReaderOnlyLockEnablesTouchInReaderOnly) {
  Inputs in = piccoReaderTouchOff();
  in.globalLock = false;
  EXPECT_EQ(routeFor(in), Route::EnableTouch);
  in.isReader = false;  // Touch already works outside the reader there.
  EXPECT_EQ(routeFor(in), Route::MappedAction);
}

TEST(TouchscreenEscapeHatch, TouchOnRunsMappedAction) {
  Inputs in = piccoReaderTouchOff();
  in.touchDisabled = false;
  EXPECT_EQ(routeFor(in), Route::MappedAction);
  in.isReader = false;
  EXPECT_EQ(routeFor(in), Route::MappedAction);
}

TEST(TouchscreenEscapeHatch, ModalQuickLockAndNoTouchHardwareNeverEnableTouch) {
  Inputs in = piccoReaderTouchOff();
  in.modalOwnsInput = true;
  EXPECT_EQ(routeFor(in), Route::MappedAction);
  in = piccoReaderTouchOff();
  in.quickLocked = true;
  EXPECT_EQ(routeFor(in), Route::MappedAction);
  in = piccoReaderTouchOff();
  in.hasTouchHardware = false;
  EXPECT_EQ(routeFor(in), Route::MappedAction);
}
