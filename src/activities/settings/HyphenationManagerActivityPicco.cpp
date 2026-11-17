// Picco hyphenation packs (T_HyphenationPacks, T_HyphenationPacks2).
#include "HyphenationManagerActivity.h"

#if CROSSINK_APP_PICCO_UI

#include <Epub/hyphenation/LanguageRegistry.h>
#include <GfxRenderer.h>
#include <I18n.h>

#include <vector>

#include "HyphenationPackStore.h"
#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "components/icons/piccoSystemIcons.h"
#include "components/picco/PiccoSystemLists.h"

namespace {
using PiccoSettingsRows::Row;
using PiccoSettingsRows::Trailing;

constexpr int kMaxLanguages = 10;

struct Model {
  bool installed[kMaxLanguages] = {};
  int16_t order[kMaxLanguages + 2] = {};
  int rows = 0;
};

Model buildModel() {
  Model m;
  const auto languages = getLanguageEntries();
  const int count = static_cast<int>(languages.size < kMaxLanguages ? languages.size : kMaxLanguages);
  for (int i = 0; i < count; ++i) {
    const auto& entry = languages.data[i];
    m.installed[i] = entry.hyphenator != nullptr || HyphenationPackStore::isInstalled(entry.primaryTag);
  }
  m.rows = PiccoSystemLists::splitInstalled(m.installed, count, m.order);
  return m;
}

bool builtIn(const int index) {
  const auto languages = getLanguageEntries();
  return index >= 0 && static_cast<size_t>(index) < languages.size && languages.data[index].hyphenator != nullptr;
}
}  // namespace

bool HyphenationManagerActivity::usesPicco() const { return UITheme::isPicco(); }

void HyphenationManagerActivity::piccoLoop() {
  const auto redraw = [this] { requestUpdate(); };
  if (piccoActions_) {
    const auto in = piccoList_.input(
        mappedInput, renderer, buttonNavigator_, catalog_->labels[selected_], 2, piccoActionRow_,
        [](const int i) { return i == 0 || i == 1; }, redraw);
    if (in.event == PiccoListView::Event::Back) {
      piccoActions_ = false;
      piccoList_.reset();
      requestUpdate();
    } else if (in.event == PiccoListView::Event::Activate) {
      piccoActions_ = false;
      pending_ = in.index == 0 ? Pending::Connect : Pending::Remove;
    }
    return;
  }

  const Model m = buildModel();
  const auto in = piccoList_.input(
      mappedInput, renderer, buttonNavigator_, tr(STR_HYPHENATION), m.rows, piccoRow_,
      [&m](const int row) { return row >= 0 && row < m.rows && m.order[row] >= 0; }, redraw);
  switch (in.event) {
    case PiccoListView::Event::Back:
      finish();
      return;
    case PiccoListView::Event::Button:
    case PiccoListView::Event::Activate: {
      const int index = m.order[in.index];
      if (builtIn(index)) return;
      selected_ = index;
      if (!m.installed[index]) {
        select();  // Get: an SD copy installs offline, otherwise the Wi-Fi download.
      } else if (in.event == PiccoListView::Event::Button) {
        pending_ = Pending::Remove;
      } else {
        piccoActions_ = true;
        piccoActionRow_ = 0;
        piccoList_.reset();
        requestUpdate();
      }
      return;
    }
    case PiccoListView::Event::Step:
    case PiccoListView::Event::Action:
    case PiccoListView::Event::Dismiss:
    case PiccoListView::Event::None:
      return;
  }
}

void HyphenationManagerActivity::renderPicco() {
  if (piccoActions_) {
    Row rows[2];
    rows[0].label = tr(STR_UPDATE);
    rows[1].label = tr(STR_PICCO_REMOVE);
    PiccoListView::Frame frame;
    frame.title = catalog_->labels[selected_];
    frame.note = tr(STR_HYPHENATION_APPLY_HINT);
    piccoList_.draw(renderer, mappedInput, frame, rows, 2, piccoActionRow_);
    renderer.displayBuffer();
    return;
  }
  const Model m = buildModel();
  Row rows[kMaxLanguages + 2];
  for (int row = 0; row < m.rows; ++row) {
    Row& r = rows[row];
    const int index = m.order[row];
    if (index < 0) {
      r.section = true;
      r.label = PiccoSystemLists::sectionIsInstalled(m.order, m.rows, row, m.installed)
                    ? tr(STR_INSTALLED)
                    : tr(STR_PICCO_SECTION_AVAILABLE);
      continue;
    }
    r.label = catalog_->labels[index];
    if (builtIn(index)) {
      r.hint = tr(STR_HYPHENATION_BUILT_IN);
    } else if (m.installed[index]) {
      r.hint = tr(STR_INSTALLED);
      r.trailing = Trailing::Button;
      r.value = tr(STR_PICCO_REMOVE);
    } else {
      r.trailing = Trailing::Button;
      r.value = tr(STR_PICCO_GET);
      r.buttonIcon = &icon_picco_download_22;
    }
  }
  PiccoListView::Frame frame;
  frame.title = tr(STR_HYPHENATION);
  frame.note = tr(STR_PICCO_HYPHENATION_NOTE);
  piccoList_.draw(renderer, mappedInput, frame, rows, m.rows, piccoRow_);
  renderer.displayBuffer();
}

#endif
