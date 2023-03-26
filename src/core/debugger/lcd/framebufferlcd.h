#ifndef DEBUGGERFRAMEBUFFERLCD_H
#define DEBUGGERFRAMEBUFFERLCD_H

#include "lcd.h"
#include "core/lcd/framebufferlcd.h"
#include "core/debugger/io/lcd.h"

class DebuggableFrameBufferLCD : public FrameBufferLCD, public ILCDDebug {
public:
    ~DebuggableFrameBufferLCD() override = default;

    State getState() override;
};

#endif // DEBUGGERFRAMEBUFFERLCD_H