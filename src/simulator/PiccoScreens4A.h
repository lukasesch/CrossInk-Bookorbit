#pragma once
#ifdef SIMULATOR
#include <functional>
#include <string>
#include <vector>

// Lane 4A captures (Library, Files, BookOrbit, sign-in, stats) for the Picco screens
// scenario. Appends steps; each returns the main-loop ticks to wait before the next.
void addPiccoScreens4A(std::vector<std::function<int()>>& steps,
                       const std::function<void(const std::string&)>& renderAndCapture);
#endif
