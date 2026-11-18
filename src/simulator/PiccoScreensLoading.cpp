#ifdef SIMULATOR
#include "PiccoScreensLoading.h"

#include <GfxRenderer.h>

#include "AppCapabilities.h"
#include "activities/RenderLock.h"
#include "activities/reader/BookLoadingFeedback.h"

#if CROSSINK_APP_CAP_TOUCH && CROSSINK_APP_PICCO_UI
extern GfxRenderer renderer;

void addPiccoScreensLoading(std::vector<std::function<int()>>& steps, const std::string& bookPath,
                            const std::function<void(const std::string&)>& capture) {
  // Drawn straight into the frame buffer as the reader would; the next step repaints.
  steps.push_back([bookPath, capture] {
    {
      RenderLock lock;
      BookLoadingFeedback feedback;
      feedback.setBook(bookPath);
      feedback.start();
      feedback.showIfPending(renderer);
    }
    capture("loading-open");
    {
      RenderLock lock;
      BookLoadingFeedback feedback;
      feedback.setBook(bookPath);
      feedback.showIndexing(renderer);
      feedback.showProgress(renderer, 45);
    }
    capture("loading-indexing");
    return 0;
  });
}

#else

void addPiccoScreensLoading(std::vector<std::function<int()>>&, const std::string&,
                            const std::function<void(const std::string&)>&) {}

#endif
#endif
