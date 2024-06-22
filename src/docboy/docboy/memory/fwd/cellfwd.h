#ifndef CELLFWD_H
#define CELLFWD_H

#include <cstdint>

template <typename Type>
using CompositeReadFunction = uint8_t (Type::*)() const;

template <typename Type>
using CompositeWriteFunction = void (Type::*)(uint8_t);

template <uint16_t Address, typename T, CompositeReadFunction<T> Read, CompositeWriteFunction<T> Write>
class Composite;

#ifdef ENABLE_DEBUGGER
struct UInt8;
#else
using UInt8 = uint8_t;
#endif

#endif // CELLFWD_H