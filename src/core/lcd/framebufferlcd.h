#ifndef FRAMEBUFFERLCD_H
#define FRAMEBUFFERLCD_H

#include "core/definitions.h"
#include "core/lcd/lcd.h"

class IFrameBuffer {
public:
    virtual ~IFrameBuffer() = default;
    virtual uint16_t* getFrameBuffer() = 0;
};

class FrameBufferLCD : public IFrameBuffer, public LCD {
public:
    FrameBufferLCD();
    ~FrameBufferLCD() override = default;

    uint16_t* getFrameBuffer() override;

protected:
    void putPixel(Pixel pixel, uint8_t x, uint8_t y) override;

    uint16_t pixels[Specs::Display::WIDTH * Specs::Display::HEIGHT];
};

#endif // FRAMEBUFFERLCD_H