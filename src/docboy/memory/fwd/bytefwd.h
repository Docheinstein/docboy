#ifndef BYTEFWD_H
#define BYTEFWD_H

#include <cstdint>

// clang-format off
#ifdef ENABLE_DEBUGGER_MEMORY_SNIFFER
#define BYTE_DEF_2(name, address) byte name{address}
#define BYTE_DEF_3(name, address, value) byte name{address, value}
#define BYTE_DEF_PICKER(_3, _2, _1, BYTE_DEF, ...) BYTE_DEF
#define BYTE(...) BYTE_DEF_PICKER(__VA_ARGS__, BYTE_DEF_3, BYTE_DEF_2)(__VA_ARGS__)
class byte;
#else
#define BYTE_DEF_2(name, address) byte name{}
#define BYTE_DEF_3(name, address, value) byte name{value}
#define BYTE_DEF_PICKER(_3, _2, _1, BYTE_DEF, ...) BYTE_DEF
#define BYTE(...) BYTE_DEF_PICKER(__VA_ARGS__, BYTE_DEF_3, BYTE_DEF_2)(__VA_ARGS__)
using byte = uint8_t;
#endif
// clang-format on

#endif // BYTEFWD_H