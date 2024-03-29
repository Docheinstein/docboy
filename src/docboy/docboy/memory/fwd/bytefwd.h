#ifndef BYTEFWD_H
#define BYTEFWD_H

#include <cstdint>

// clang-format off
#ifdef ENABLE_DEBUGGER_MEMORY_SNIFFER
struct byte;
#else
using byte = uint8_t;
#endif

#endif // BYTEFWD_H