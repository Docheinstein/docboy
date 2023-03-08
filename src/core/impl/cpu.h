#ifndef CPUIMPL_H
#define CPUIMPL_H


#if ENABLE_DEBUGGER
#include "core/debugger/cpu/cpu.h"
namespace Impl {
using ICPU = IDebuggableCPU;
using CPU = DebuggableCPU;
}
#else
#include "core/cpu/cpu.h"
namespace Impl {
using ICPU = ICPU;
using CPU = CPU;
}
#endif

#endif // CPUIMPL_H