#ifndef CELLFWD_H
#define CELLFWD_H

#include <cstdint>

template <typename T, uint16_t Address>
struct Composite;

#ifdef ENABLE_DEBUGGER
struct UInt8;
#else
using UInt8 = uint8_t;
#endif

#endif // CELLFWD_H