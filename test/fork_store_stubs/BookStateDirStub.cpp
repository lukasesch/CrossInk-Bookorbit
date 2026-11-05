// An empty state dir stands for a book that cannot be hashed: EpubReaderUtils then
// keeps only the path-keyed progress copy that upstream's tests describe.
#include "util/BookContentId.h"

std::string BookContentId::bookStateDirName(const std::string&) { return ""; }
std::string BookContentId::bookStateDir(const std::string&) { return ""; }
