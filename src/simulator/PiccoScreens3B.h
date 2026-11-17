#pragma once
#ifdef SIMULATOR

#include <functional>
#include <string>
#include <vector>

// Phase 3b captures (the Picco reader sheet) appended to the Picco screens scenario.
struct PiccoScreenHooks {
  std::vector<std::function<int()>>* steps = nullptr;
  std::function<void(const std::string&)> renderAndCapture;
  std::function<void(const char* activity, const char* what)> expect;
  int settleTicks = 6;
  int bookOpenTicks = 40;
};

void addPiccoSheetScreens(const PiccoScreenHooks& hooks, const std::string& bookPath);

#endif
