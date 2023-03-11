#ifndef FRAMEBUFFERLCD_H
#define FRAMEBUFFERLCD_H

#include "core/impl/lcd.h"

class IFrameBufferLCD : public virtual Impl::ILCD {
public:
    virtual uint32_t *getFrameBuffer() = 0;
};

class FrameBufferLCD : public virtual IFrameBufferLCD, public virtual Impl::LCD {
public:
    FrameBufferLCD();
    ~FrameBufferLCD() override = default;

    uint32_t *getFrameBuffer() override;

protected:
    void putPixel(Pixel pixel, uint8_t x, uint8_t y) override;

    uint32_t pixels[Specs::Display::WIDTH * Specs::Display::HEIGHT];
};
#endif // FRAMEBUFFERLCD_H