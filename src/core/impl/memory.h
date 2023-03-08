#ifndef MEMORYIMPL_H
#define MEMORYIMPL_H


#if ENABLE_DEBUGGER
#include "core/debugger/memory/memory.h"
namespace Impl {
using IMemory = IDebuggableMemory;
using Memory = DebuggableMemory;
}
#else
#include "core/memory/memory.h"
namespace Impl {
using IMemory = IMemory;
using Memory = Memory;
}
#endif

#endif // MEMORYIMPL_H