#pragma once

#include <cstdint>

// Which overlay a reader's openReaderMenu() shows. Drawer is today's menu
// (the EPUB drawer, the XTC menu list); Sheet is reserved for the reader sheet.
enum class ReaderMenuKind : uint8_t { Drawer, Sheet };

constexpr ReaderMenuKind readerMenuKind() { return ReaderMenuKind::Drawer; }
