#ifndef LCD_H
#define LCD_H

#include <cstdint>
#include "core/io/lcd.h"

class ILCD {
public:
    enum class Pixel {
        Color0,
        Color1,
        Color2,
        Color3
    };

    virtual void pushPixel(Pixel pixel) = 0;
    virtual void turnOn() = 0;
    virtual void turnOff() = 0;
};

class LCD : public ILCD {
public:
    LCD();

    void pushPixel(Pixel pixel) override;
    void turnOn() override;
    void turnOff() override;

protected:
    virtual void putPixel(Pixel pixel, uint8_t x, uint8_t y);

    void reset();

    uint8_t x;
    uint8_t y;
};

#endif // LCD_H