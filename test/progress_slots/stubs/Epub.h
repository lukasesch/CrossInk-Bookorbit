#pragma once
#include <string>
class Epub {
 public:
  const std::string& getCachePath() const {
    static const std::string path = "/epub";
    return path;
  }
  // This fork's content-keyed progress copy asks for the book path.
  const std::string& getPath() const {
    static const std::string path = "/book.epub";
    return path;
  }
};
