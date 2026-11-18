#pragma once
#ifdef SIMULATOR
#include <functional>
#include <string>
#include <vector>

// Lane 4R1 captures (Contents tabs, book stats, lookup history, end of book) for the Picco
// screens scenario. capture writes the frame buffer as it is; renderAndCapture renders first.
void addPiccoScreens4R1(std::vector<std::function<int()>>& steps, const std::string& bookPath,
                        const std::function<void(const std::string&)>& capture,
                        const std::function<void(const std::string&)>& renderAndCapture);
#endif
