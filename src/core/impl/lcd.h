#ifndef LCDIMPL_H
#define LCDIMPL_H

#if ENABLE_DEBUGGER
#include "core/debugger/ppu/lcd.h"
namespace Impl {
using ILCD = IDebuggableLCD;
using LCD = DebuggableLCD;
}
#else
#include "core/ppu/lcd.h"
namespace Impl {
using ILCD = ILCD;
using LCD = LCD;
}
#endif

#endif // LCDIMPL_H