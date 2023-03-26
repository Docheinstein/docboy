#include "framebufferlcd.h"

ILCDDebug::State DebuggableFrameBufferLCD::getState() {
    return {
        .x = x,
        .y = y
    };
}
