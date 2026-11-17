// Picco font list (T_Fonts): Installed families (the reading font checked), Available with a
// Get button, Update all under More. Installed rows keep the delete prompt on activation.
// Deviation: the built-in families are not in the manifest, so they are not listed here.
#include "FontDownloadActivity.h"

#if CROSSINK_APP_PICCO_UI

#include <GfxRenderer.h>
#include <I18n.h>
#include <Logging.h>
#include <Memory.h>

#include <cstring>
#include <vector>

#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "components/icons/piccoSystemIcons.h"
#include "components/picco/PiccoSystemLists.h"

namespace {
using PiccoSettingsRows::Row;
using PiccoSettingsRows::Trailing;
}  // namespace

bool FontDownloadActivity::usesPiccoList() const { return UITheme::isPicco(); }

int FontDownloadActivity::piccoOrder(int16_t* order, const int capacity) const {
  const int families = static_cast<int>(manifestFamilyCount_);
  if (families <= 0 || capacity < families + 4) return 0;
  // Cold path, one per input or redraw: a flag per family for the split.
  auto installed = makeUniqueNoThrow<bool[]>(families);
  if (!installed) {
    LOG_ERR("FONT", "OOM: %d font list flags", families);
    return 0;
  }
  for (int i = 0; i < families; ++i) installed[i] = manifestFamilies_[i].installed;
  int rows = PiccoSystemLists::splitInstalled(installed.get(), families, order);
  const int special = specialRowCount();
  for (int i = 0; i < rows; ++i) {
    if (order[i] >= 0) order[i] = static_cast<int16_t>(order[i] + special);
  }
  if (showUpdateAllRow()) {
    order[rows++] = PiccoSystemLists::kSection;
    order[rows++] = 0;  // the Update all row
  }
  return rows;
}

void FontDownloadActivity::piccoListLoop() {
  std::vector<int16_t> order(manifestFamilyCount_ + 4);
  const int rows = piccoOrder(order.data(), static_cast<int>(order.size()));
  const auto in = piccoList_.input(
      mappedInput, renderer, buttonNavigator_, tr(STR_PICCO_FONTS), rows, piccoRow_,
      [&order, rows](const int row) { return row >= 0 && row < rows && order[row] >= 0; }, [this] { requestUpdate(); });
  switch (in.event) {
    case PiccoListView::Event::Back:
      finishAfterBackPress();
      return;
    case PiccoListView::Event::Activate:
    case PiccoListView::Event::Button:
      selectedIndex_ = order[in.index];
      activateSelected();
      return;
    case PiccoListView::Event::Step:
    case PiccoListView::Event::Action:
    case PiccoListView::Event::Dismiss:
    case PiccoListView::Event::None:
      return;
  }
}

void FontDownloadActivity::renderPiccoList() {
  std::vector<int16_t> order(manifestFamilyCount_ + 4);
  const int count = piccoOrder(order.data(), static_cast<int>(order.size()));
  std::vector<Row> rows(count);
  const int special = specialRowCount();
  for (int row = 0; row < count; ++row) {
    Row& r = rows[row];
    const int listIndex = order[row];
    if (listIndex < 0) {
      const bool more = row + 1 < count && isUpdateAllRow(order[row + 1]);
      const bool installed = !more && row + 1 < count && manifestFamilies_[order[row + 1] - special].installed;
      r.section = true;
      r.label = more ? tr(STR_PICCO_SECTION_MORE) : (installed ? tr(STR_INSTALLED) : tr(STR_PICCO_SECTION_AVAILABLE));
      continue;
    }
    if (isUpdateAllRow(listIndex)) {
      r.label = updateAllLabel_;
      r.trailing = Trailing::Chevron;
      continue;
    }
    const ManifestFamily& family = manifestFamilies_[listIndex - special];
    r.label = family.name;
    r.hint = family.description[0] != '\0' ? family.description : nullptr;
    if (family.hasUpdate) {
      r.trailing = Trailing::Button;
      r.value = tr(STR_UPDATE);
    } else if (family.installed) {
      r.trailing = Trailing::Check;
      r.on = !family.installName.empty() && family.installName == SETTINGS.sdFontFamilyName;
      r.bold = r.on;
    } else {
      r.trailing = Trailing::Button;
      r.value = tr(STR_PICCO_GET);
      r.buttonIcon = &icon_picco_download_22;
    }
  }
  if (piccoRow_ < 0 || piccoRow_ >= count || order[piccoRow_] < 0) piccoRow_ = count > 1 ? 1 : 0;
  PiccoListView::Frame frame;
  frame.title = tr(STR_PICCO_FONTS);
  frame.note = count == 0 ? tr(STR_NO_FONTS_AVAILABLE) : nullptr;
  piccoList_.draw(renderer, mappedInput, frame, rows.data(), count, piccoRow_);
}

#endif
