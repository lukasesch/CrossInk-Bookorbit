#include <gtest/gtest.h>

#include <cstdint>
#include <iterator>

#include "CrossPointSettings.h"
#include "components/picco/PiccoSegmented.h"

using Settings = CrossPointSettings;

namespace {
const Settings::QuickActionsBinding& bindingFor(uint8_t Settings::* field) {
  for (const auto& binding : Settings::QUICK_ACTIONS_BINDINGS) {
    if (binding.field == field) return binding;
  }
  ADD_FAILURE() << "field has no Quick Actions binding";
  return Settings::QUICK_ACTIONS_BINDINGS[0];
}
}  // namespace

// Persisted numbers: a renumbered value would silently rebind saved shortcuts.
static_assert(Settings::QUICK_ACTIONS == 27);
static_assert(Settings::CHORD_QUICK_ACTIONS == 26);
static_assert(Settings::LONG_MENU_QUICK_ACTIONS == 22);

TEST(PiccoQuickActions, EveryShortcutFieldIsCoveredOnce) {
  for (const auto& binding : Settings::QUICK_ACTIONS_BINDINGS) {
    int owners = 0;
    for (const auto& other : Settings::QUICK_ACTIONS_BINDINGS) owners += other.field == binding.field ? 1 : 0;
    EXPECT_EQ(owners, 1);
    EXPECT_NE(binding.openQuickActions, binding.piccoReplacement);
  }
  // Same fields as QuickActions::triggerForSetting().
  EXPECT_EQ(std::size(Settings::QUICK_ACTIONS_BINDINGS), 15u);
}

TEST(PiccoQuickActions, HomeKeyOpensTheReaderMenu) {
  for (const auto field :
       {&Settings::homeButtonTapAction, &Settings::homeButtonDoubleTapAction, &Settings::homeButtonLongPressAction}) {
    EXPECT_EQ(Settings::withoutQuickActions(bindingFor(field), Settings::QUICK_ACTIONS),
              Settings::HOME_BUTTON_READER_MENU);
  }
}

TEST(PiccoQuickActions, FieldsWithoutAReaderMenuValueBecomeNone) {
  for (const auto field : {&Settings::shortPwrBtn, &Settings::longPwrBtn, &Settings::sideButtonUpShort,
                           &Settings::sideButtonUpLong, &Settings::sideButtonDownShort, &Settings::sideButtonDownLong,
                           &Settings::frontButtonLeftLong, &Settings::frontButtonRightLong}) {
    EXPECT_EQ(Settings::withoutQuickActions(bindingFor(field), Settings::QUICK_ACTIONS), Settings::IGNORE);
  }
  for (const auto field : {&Settings::powerChordAction, &Settings::sideButtonChordAction}) {
    EXPECT_EQ(Settings::withoutQuickActions(bindingFor(field), Settings::CHORD_QUICK_ACTIONS),
              Settings::CHORD_DISABLED);
  }
  for (const auto field : {&Settings::longPressMenuAction, &Settings::longPressBackAction}) {
    EXPECT_EQ(Settings::withoutQuickActions(bindingFor(field), Settings::LONG_MENU_QUICK_ACTIONS),
              Settings::LONG_MENU_OFF);
  }
}

TEST(PiccoQuickActions, OtherValuesAreKept) {
  // 27 means Select Chapter on a long press and 26 Toggle Home Key on a power button.
  EXPECT_EQ(Settings::withoutQuickActions(bindingFor(&Settings::longPressMenuAction), 27),
            Settings::LONG_MENU_SELECT_CHAPTER);
  EXPECT_EQ(Settings::withoutQuickActions(bindingFor(&Settings::shortPwrBtn), Settings::TOGGLE_HOME_BUTTON_IN_READER),
            Settings::TOGGLE_HOME_BUTTON_IN_READER);
  EXPECT_EQ(Settings::withoutQuickActions(bindingFor(&Settings::powerChordAction), Settings::CHORD_SCREENSHOT),
            Settings::CHORD_SCREENSHOT);
  EXPECT_EQ(Settings::withoutQuickActions(bindingFor(&Settings::homeButtonTapAction), Settings::QUICK_LOCK),
            Settings::QUICK_LOCK);
}

TEST(PiccoDefaultTheme, FreshSettingsStartOnPiccoOnlyInPiccoBuilds) {
  EXPECT_EQ(Settings::DEFAULT_UI_THEME, PICCO_UI_EXPECT_ON ? Settings::PICCO : Settings::LYRA);
  EXPECT_TRUE(Settings::isKnownUiTheme(Settings::DEFAULT_UI_THEME));
}

namespace Seg = PiccoSegmented;
using PiccoLayout::Box;

TEST(PiccoSegmented, EqualCellsWithFourPixelPads) {
  const Box frame{24, 100, 432, 64};
  const Box first = Seg::cell(frame, 2, 0);
  const Box second = Seg::cell(frame, 2, 1);
  EXPECT_EQ(first.x, 29);
  EXPECT_EQ(first.y, 105);
  EXPECT_EQ(first.height, PiccoTokens::kTabCell);
  EXPECT_EQ(first.width, 209);
  EXPECT_EQ(second.x, first.right() + PiccoTokens::kTabPad);
  EXPECT_EQ(second.right(), frame.right() - Seg::kInset);
  EXPECT_TRUE(Seg::cell(frame, 2, 2).empty());
  EXPECT_TRUE(Seg::cell(frame, 0, 0).empty());
}

TEST(PiccoSegmented, LastCellTakesTheRemainder) {
  const Box frame{0, 0, 101, 64};  // 83 inner px over 3 cells: 27, 27, 29
  EXPECT_EQ(Seg::cell(frame, 3, 0).width, 27);
  EXPECT_EQ(Seg::cell(frame, 3, 2).right(), frame.right() - Seg::kInset);
  EXPECT_EQ(Seg::cell(frame, 3, 2).width, 29);
}

TEST(PiccoSegmented, FixedCellsFillTheirFrame) {
  const Box row{24, 200, 432, 80};
  for (int count = 1; count <= 5; ++count) {
    const Box frame = Seg::rightAligned(row, count, 60);
    EXPECT_EQ(frame.right(), row.right());
    EXPECT_EQ(frame.y, 208);
    for (int i = 0; i < count; ++i) EXPECT_EQ(Seg::cell(frame, count, i).width, 60) << count << "/" << i;
  }
}

TEST(PiccoSegmented, HitTestCoversTheWholeFrame) {
  const Box frame{24, 100, 432, 64};
  EXPECT_EQ(Seg::at(frame, 2, 24, 100), 0);
  EXPECT_EQ(Seg::at(frame, 2, 239, 130), 0);  // left half of the pad
  EXPECT_EQ(Seg::at(frame, 2, 240, 130), 1);
  EXPECT_EQ(Seg::at(frame, 2, 455, 163), 1);
  EXPECT_EQ(Seg::at(frame, 2, 456, 130), -1);
  EXPECT_EQ(Seg::at(frame, 2, 100, 99), -1);
}
