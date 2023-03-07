#ifndef COMPONENTS_H
#define COMPONENTS_H

#if ENABLE_DEBUGGER
#include "debugger/debuggablecpu.h"
#include "debugger/debuggableppu.h"
#include "debugger/debuggablelcd.h"
#include "debugger/debuggablememory.h"
#define ICPUImpl IDebuggableCPU
#define IPPUImpl IDebuggablePPU
#define ILCDImpl IDebuggableLCD
#define IReadableImpl IDebuggableReadable
#define IMemoryImpl IDebuggableMemory

#define CPUImpl DebuggableCPU
#define PPUImpl DebuggablePPU
#define LCDImpl DebuggableLCD
#define ReadableImpl DebuggableReadable
#define MemoryImpl DebuggableMemory

#else
#include "core/cpu/cpu.h"
#include "core/ppu/ppu.h"
#include "core/ppu/lcd.h"
#include "core/memory.h"

#define ICPUImpl ICPU
#define IPPUImpl IPPU
#define ILCDImpl ILCD
#define IReadableImpl IReadable
#define IMemoryImpl IMemory

#define CPUImpl CPU
#define PPUImpl PPU
#define LCDImpl LCD
#define ReadableImpl Readable
#define MemoryImpl Memory
#endif

#endif // COMPONENTS_H