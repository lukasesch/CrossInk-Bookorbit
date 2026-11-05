// Link stubs for upstream tests that compile this fork's content-keyed stores. An empty
// content hash stands for a book that cannot be hashed: the stores then fall back to the
// path-keyed names those tests describe.
#include <WallClock.h>

#include "util/BookContentId.h"

std::string BookContentId::contentHash(const std::string&) { return ""; }

bool WallClock::now(uint32_t& outEpochSeconds) {
  outEpochSeconds = 0;
  return false;
}
