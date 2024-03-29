#pragma once

#include "driver/platform/platform.h"
#include "driver/utils/include/folly_memory_UninitializedMemoryHacks.h"

#define resize_without_initialization(container, size) folly::resizeWithoutInitialization(container, size)

//FOLLY_DECLARE_STRING_RESIZE_WITHOUT_INIT(char)
FOLLY_DECLARE_STRING_RESIZE_WITHOUT_INIT(signed char)
FOLLY_DECLARE_STRING_RESIZE_WITHOUT_INIT(unsigned char)
//FOLLY_DECLARE_STRING_RESIZE_WITHOUT_INIT(char8_t)
FOLLY_DECLARE_STRING_RESIZE_WITHOUT_INIT(char16_t)
FOLLY_DECLARE_STRING_RESIZE_WITHOUT_INIT(char32_t)
//FOLLY_DECLARE_STRING_RESIZE_WITHOUT_INIT(wchar_t)
FOLLY_DECLARE_STRING_RESIZE_WITHOUT_INIT(unsigned short)
