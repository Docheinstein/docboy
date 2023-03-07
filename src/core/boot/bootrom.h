#ifndef BOOTROM_H
#define BOOTROM_H

#if ENABLE_DEBUGGER
#include "core/debugger/debuggablememory.h"
#define IBootROM IDebuggableReadable
#define BootROM DebuggableReadable
#else
#include "core/memory.h"
#define IBootROM IReadable
#define BootROM Readable
#endif

#endif // BOOTROM_H