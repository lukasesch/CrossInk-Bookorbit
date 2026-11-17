#include <gtest/gtest.h>

#include <cstring>

#include "PendingOverlayResume.h"
#include "ReaderMenuKind.h"
#include "ReaderSettingsEdits.h"

using RowId = ReaderDrawerCatalogItem;

TEST(ReaderSettingsEdits, TogglesFlipTheDraftAndReportTheirChange) {
  ReaderSettingsDraft draft;
  EXPECT_EQ(toggleReaderSetting(draft, RowId::TextAa),
            ReaderSettingsChangeMask::Preview | ReaderSettingsChangeMask::NonLayout);
  EXPECT_EQ(draft.textAntiAliasing, 1);
  EXPECT_EQ(toggleReaderSetting(draft, RowId::Focus),
            ReaderSettingsChangeMask::Preview | ReaderSettingsChangeMask::Relayout);
  EXPECT_EQ(draft.focusReadingEnabled, 1);
  EXPECT_EQ(toggleReaderSetting(draft, RowId::Hyphenation), ReaderSettingsChangeMask::Relayout);
  EXPECT_EQ(draft.hyphenationEnabled, 1);
  EXPECT_EQ(toggleReaderSetting(draft, RowId::Hyphenation), ReaderSettingsChangeMask::Relayout);
  EXPECT_EQ(draft.hyphenationEnabled, 0);
  EXPECT_EQ(toggleReaderSetting(draft, RowId::IndexingMethod), ReaderSettingsChangeMask::Relayout);
  EXPECT_EQ(draft.indexingMethod, 1);
}

TEST(ReaderSettingsEdits, NonToggleRowsLeaveTheDraftAlone) {
  ReaderSettingsDraft draft;
  draft.readerFontPointSize = 14;
  EXPECT_EQ(toggleReaderSetting(draft, RowId::FontSize), ReaderSettingsChangeMask::None);
  EXPECT_EQ(toggleReaderSetting(draft, RowId::SelectChapter), ReaderSettingsChangeMask::None);
  EXPECT_EQ(draft.readerFontPointSize, 14);
}

TEST(ReaderSettingsEdits, EnumOptionsWriteTheirDraftField) {
  ReaderSettingsDraft draft;
  EXPECT_EQ(applyReaderEnumOption(draft, RowId::FontSize, 18),
            ReaderSettingsChangeMask::Preview | ReaderSettingsChangeMask::Relayout);
  EXPECT_EQ(draft.readerFontPointSize, 18);
  EXPECT_EQ(applyReaderEnumOption(draft, RowId::Orientation, 2), ReaderSettingsChangeMask::Orientation);
  EXPECT_EQ(draft.orientation, 2);
  EXPECT_EQ(applyReaderEnumOption(draft, RowId::Alignment, 3),
            ReaderSettingsChangeMask::Preview | ReaderSettingsChangeMask::Relayout);
  EXPECT_EQ(draft.paragraphAlignment, 3);
  EXPECT_EQ(applyReaderEnumOption(draft, RowId::RenderMode, 1), ReaderSettingsChangeMask::Relayout);
  EXPECT_EQ(draft.epubRenderMode, 1);
  EXPECT_EQ(applyReaderEnumOption(draft, RowId::Images, ImageDisplayMode::Placeholder),
            ReaderSettingsChangeMask::NonLayout);
  EXPECT_EQ(draft.imageRendering, 1);
  EXPECT_EQ(applyReaderEnumOption(draft, RowId::DictionaryFontSize, 12), ReaderSettingsChangeMask::None);
}

TEST(ReaderSettingsEdits, OnlyFontSizeAndAlignmentPreviewBeforeSelecting) {
  EXPECT_TRUE(readerEnumOptionPreviewsFirst(RowId::FontSize));
  EXPECT_TRUE(readerEnumOptionPreviewsFirst(RowId::Alignment));
  EXPECT_FALSE(readerEnumOptionPreviewsFirst(RowId::Orientation));
  EXPECT_FALSE(readerEnumOptionPreviewsFirst(RowId::DictionaryFontFamily));
}

TEST(ReaderSettingsEdits, ChangeStateAccumulatesMasksAndKeepsDidChangeAfterCommit) {
  ReaderSettingsChangeState changes;
  changes.mark(ReaderSettingsChangeMask::Preview);
  changes.mark(ReaderSettingsChangeMask::Orientation);
  EXPECT_TRUE(changes.settingsChanged);
  EXPECT_TRUE(changes.didChangeSettings);
  EXPECT_TRUE(hasReaderSettingsChange(changes.changeMask, ReaderSettingsChangeMask::Preview));
  EXPECT_TRUE(hasReaderSettingsChange(changes.changeMask, ReaderSettingsChangeMask::Orientation));
  EXPECT_FALSE(hasReaderSettingsChange(changes.changeMask, ReaderSettingsChangeMask::Relayout));
}

TEST(ReaderSettingsEdits, CommitIsANoOpWithoutChanges) {
  ReaderSettingsDraft draft;
  std::strcpy(draft.sdFontFamilyName.data(), "Unpreviewed");
  const ReaderSettingsDraft lastGood;
  EXPECT_EQ(prepareReaderSettingsCommit(false, true, draft, lastGood), ReaderSettingsCommit::Unchanged);
  EXPECT_STREQ(draft.sdFontFamilyName.data(), "Unpreviewed");
}

TEST(ReaderSettingsEdits, CommitSavesChangedDraft) {
  ReaderSettingsDraft draft;
  ReaderSettingsChangeState changes;
  changes.mark(toggleReaderSetting(draft, RowId::Hyphenation));
  EXPECT_EQ(prepareReaderSettingsCommit(changes.settingsChanged, true, draft, ReaderSettingsDraft{}),
            ReaderSettingsCommit::Save);
  EXPECT_EQ(draft.hyphenationEnabled, 1);
}

TEST(ReaderSettingsEdits, ButtonCommitKeepsOnlyAPreviewedSdFont) {
  ReaderSettingsDraft lastGood;
  lastGood.fontFamily = 1;
  lastGood.readerFontPointSize = 14;
  ReaderSettingsDraft draft = lastGood;
  std::strcpy(draft.sdFontFamilyName.data(), "Unpreviewed");
  draft.readerFontPointSize = 20;
  draft.hyphenationEnabled = 1;

  ReaderSettingsDraft touchDraft = draft;
  EXPECT_EQ(prepareReaderSettingsCommit(true, false, touchDraft, lastGood), ReaderSettingsCommit::Save);
  EXPECT_STREQ(touchDraft.sdFontFamilyName.data(), "Unpreviewed");

  EXPECT_EQ(prepareReaderSettingsCommit(true, true, draft, lastGood), ReaderSettingsCommit::SaveWithPreviousFont);
  EXPECT_EQ(draft.sdFontFamilyName[0], '\0');
  EXPECT_EQ(draft.readerFontPointSize, 14);
  EXPECT_EQ(draft.hyphenationEnabled, 1);

  ReaderSettingsDraft previewed = draft;
  std::strcpy(previewed.sdFontFamilyName.data(), "Previewed");
  const ReaderSettingsDraft previewedLastGood = previewed;
  EXPECT_EQ(prepareReaderSettingsCommit(true, true, previewed, previewedLastGood), ReaderSettingsCommit::Save);
}

TEST(PendingOverlayResume, ReaderSheetIsAppendedAndOpensTheReaderMenu) {
  EXPECT_EQ(static_cast<uint8_t>(PendingOverlayType::ReaderDrawer), 1);
  EXPECT_EQ(static_cast<uint8_t>(PendingOverlayType::FrontlightDrawer), 2);
  EXPECT_EQ(static_cast<uint8_t>(PendingOverlayType::ReaderSheet), 3);
  EXPECT_TRUE(pendingOverlayOpensReaderMenu(PendingOverlayType::ReaderDrawer));
  EXPECT_TRUE(pendingOverlayOpensReaderMenu(PendingOverlayType::ReaderSheet));
  EXPECT_FALSE(pendingOverlayOpensReaderMenu(PendingOverlayType::FrontlightDrawer));
  EXPECT_FALSE(pendingOverlayOpensReaderMenu(PendingOverlayType::None));
}

TEST(PendingOverlayResume, TopSheetsKeepTheReaderOrientation) {
  EXPECT_TRUE(pendingOverlayIsTopSheet(PendingOverlayType::FrontlightDrawer));
  EXPECT_TRUE(pendingOverlayIsTopSheet(PendingOverlayType::ReaderSheet));
  EXPECT_FALSE(pendingOverlayIsTopSheet(PendingOverlayType::ReaderDrawer));
}

TEST(ReaderMenuKind, DrawerOutsideThePiccoUi) { EXPECT_EQ(readerMenuKind(), ReaderMenuKind::Drawer); }

TEST(ReaderMenuKind, PiccoThemeOpensTheSheet) {
  EXPECT_EQ(readerMenuKindFor(true), ReaderMenuKind::Sheet);
  EXPECT_EQ(readerMenuKindFor(false), ReaderMenuKind::Drawer);
}
