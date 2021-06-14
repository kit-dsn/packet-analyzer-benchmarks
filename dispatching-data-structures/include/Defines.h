#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <functional>
#include <cassert>
#include "analyzers/IAnalyzer.h"

#ifndef DEBUG
#define DEBUG 0
#endif

#define MAX_IDENTIFIERS (uint64_t(1u) << sizeof(identifier_t) * 8)
#define PRINT_UINT_HEX(stream, value, size) stream << std::hex << std::uppercase << std::setw(size) << std::setfill('0') << value << std::dec

using Value = std::pair<identifier_t, IAnalyzer*>;

#if DEBUG > 0
#include <iostream>
#include <iomanip>
#endif