#ifndef BOOTROMIMPL_H
#define BOOTROMIMPL_H


#if ENABLE_DEBUGGER
#include "core/debugger/boot/bootrom.h"
namespace Impl {
using IBootROM = IDebuggableBootROM;
using BootROM = DebuggableBootROM;
}
#else
#include "core/boot/bootrom.h"
namespace Impl {
using IBootROM = IBootROM;
using BootROM = BootROM;
}
#endif

#endif // BOOTROMIMPL_H