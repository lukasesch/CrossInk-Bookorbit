#pragma once
#ifdef SIMULATOR
#include <functional>
#include <string>
#include <vector>

// Book loading screen captures (board L_B) for the Picco screens scenario: the quick open
// and indexing with progress. capture writes the frame buffer as it is.
void addPiccoScreensLoading(std::vector<std::function<int()>>& steps, const std::string& bookPath,
                            const std::function<void(const std::string&)>& capture);
#endif
