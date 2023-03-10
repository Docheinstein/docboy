#ifndef NOGUILCD_H
#define NOGUILCD_H

#include "core/impl/lcd.h"

class NoGuiLCD : public Impl::LCD {
public:
    NoGuiLCD() = default;

    [[nodiscard]] bool isOn() const override;
    void turnOn() override;
    void turnOff() override;

protected:
    void putPixel(Pixel pixel, uint8_t x, uint8_t y) override;
};

#endif // NOGUILCD_H