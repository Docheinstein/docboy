#ifndef DEBUGGERFRAMEBUFFERLCD_H
#define DEBUGGERFRAMEBUFFERLCD_H

#include "core/debugger/io/lcd.h"
#include "core/lcd/framebufferlcd.h"
#include "lcd.h"

class DebuggableFrameBufferLCD : public FrameBufferLCD, public ILCDDebug {
public:
    ~DebuggableFrameBufferLCD() override = default;

    State getState() override;
};

#endif // DEBUGGERFRAMEBUFFERLCD_H