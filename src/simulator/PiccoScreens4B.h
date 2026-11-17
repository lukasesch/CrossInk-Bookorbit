#pragma once
#ifdef SIMULATOR
#include <functional>
#include <string>
#include <vector>

// Lane 4B captures (Settings and system UI), appended to the Picco screens scenario.
// Each step returns the main-loop ticks to wait before the next one.
void addPiccoScreens4B(std::vector<std::function<int()>>& steps,
                       const std::function<void(const std::string&)>& renderAndCapture);
#endif
