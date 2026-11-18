#include <gtest/gtest.h>

#include "components/picco/PiccoLoadingLayout.h"

// Book loading screen (board L_B).
using PiccoLayout::Box;

TEST(PiccoLoading, FollowsTheBoard) {
  const auto l = PiccoLoadingLayout::layout(Box{0, 0, 480, 800});
  EXPECT_EQ(l.cover.x, 140);
  EXPECT_EQ(l.cover.y, 120);
  EXPECT_EQ(l.cover.width, 200);
  EXPECT_EQ(l.cover.height, 300);
  EXPECT_EQ(l.title.y, 456);
  EXPECT_EQ(l.title.height, 36);
  EXPECT_EQ(l.title.x, 24);
  EXPECT_EQ(l.title.width, 432);
  EXPECT_EQ(l.author.y, 492);
  EXPECT_EQ(l.author.height, 26);
  EXPECT_EQ(l.bar.x, 88);
  EXPECT_EQ(l.bar.right(), 392);
  EXPECT_EQ(l.bar.y, 620);
  EXPECT_EQ(l.bar.height, PiccoTokens::kProgressFill);
  EXPECT_EQ(l.status.y, 639);
}

TEST(PiccoLoading, ProgressUpdateCoversBarAndStatusOnly) {
  const auto l = PiccoLoadingLayout::layout(Box{0, 0, 480, 800});
  EXPECT_EQ(l.progressArea.y, l.bar.y);
  EXPECT_EQ(l.progressArea.bottom(), l.status.bottom());
  EXPECT_LE(l.progressArea.x, l.bar.x);
  EXPECT_GE(l.progressArea.right(), l.bar.right());
  EXPECT_FALSE(l.progressArea.overlaps(l.author));
  EXPECT_FALSE(l.progressArea.overlaps(l.cover));
  EXPECT_LE(l.progressArea.bottom(), 800);
}
