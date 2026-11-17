#include <gtest/gtest.h>

#include <cstdint>
#include <iterator>
#include <set>

#include "CrossPointSettings.h"

using Settings = CrossPointSettings;

// Persisted raw value: renumbering would silently switch every saved Picco theme.
static_assert(Settings::PICCO == 64);

TEST(PiccoUiTheme, UpstreamRangeIsKnown) {
  for (unsigned raw = 0; raw < Settings::UI_THEME_COUNT; ++raw) EXPECT_TRUE(Settings::isKnownUiTheme(raw)) << raw;
}

TEST(PiccoUiTheme, ValuesOutsideTheKnownSetAreUnknown) {
  for (unsigned raw = Settings::UI_THEME_COUNT; raw <= 255; ++raw) {
    if (raw == Settings::PICCO) continue;
    EXPECT_FALSE(Settings::isKnownUiTheme(raw)) << raw;
  }
}

TEST(PiccoUiTheme, PiccoIsKnownOnlyWhenBuilt) {
  EXPECT_EQ(Settings::isKnownUiTheme(Settings::PICCO), PICCO_UI_EXPECT_ON == 1);
}

TEST(PiccoUiTheme, KnownThemeListMatchesTheBuild) {
  EXPECT_EQ(std::size(Settings::KNOWN_UI_THEMES), PICCO_UI_EXPECT_ON ? 10u : 9u);
  std::set<uint8_t> seen;
  for (const uint8_t theme : Settings::KNOWN_UI_THEMES) {
    EXPECT_TRUE(Settings::isKnownUiTheme(theme)) << int(theme);
    EXPECT_TRUE(seen.insert(theme).second) << "duplicate " << int(theme);
  }
}

TEST(PiccoUiTheme, LegacyLoaderKeepsKnownAndMapsTheRestToLyra) {
  EXPECT_EQ(Settings::sanitizeLegacyUiTheme(Settings::BOOKORBIT), Settings::BOOKORBIT);
  EXPECT_EQ(Settings::sanitizeLegacyUiTheme(Settings::PICCO), PICCO_UI_EXPECT_ON ? Settings::PICCO : Settings::LYRA);
  EXPECT_EQ(Settings::sanitizeLegacyUiTheme(9), Settings::LYRA);
  EXPECT_EQ(Settings::sanitizeLegacyUiTheme(200), Settings::LYRA);
}
