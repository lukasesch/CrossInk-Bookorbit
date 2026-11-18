#pragma once
#ifdef SIMULATOR
#include <functional>
#include <string>
#include <vector>

// Lane 4R2 captures (word lookup, footnote, clipping selection, QR pane, sleep) for the
// Picco screens scenario. capture writes the frame buffer as it is, without a render.
void addPiccoScreens4R2(std::vector<std::function<int()>>& steps,
                        const std::function<void(const std::string&)>& renderAndCapture,
                        const std::function<void(const std::string&)>& capture, const std::string& bookPath);
#endif
