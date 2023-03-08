#ifndef READABLEIMPL_H
#define READABLEIMPL_H

#if ENABLE_DEBUGGER
#include "core/debugger/memory/readable.h"
namespace Impl {
using IReadable = IDebuggableReadable;
using Readable = DebuggableReadable;
}
#else
#include "core/memory/readable.h"
namespace Impl {
using IMemory = IMemory;
using Memory = Memory;
}
#endif

#endif // READABLEIMPL_H