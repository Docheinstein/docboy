#ifndef LCD_H
#define LCD_H

#include <cstdint>
#include "core/definitions.h"

class ILCD {
public:
    enum class Pixel {
        Color0,
        Color1,
        Color2,
        Color3
    };

    virtual ~ILCD() = default;

    virtual void pushPixel(Pixel pixel) = 0;

    [[nodiscard]] virtual bool isOn() const = 0;
    virtual void turnOn() = 0;
    virtual void turnOff() = 0;
};

class LCD : public virtual ILCD {
public:
    LCD();
    ~LCD() override = default;

    void pushPixel(Pixel pixel) override;

    [[nodiscard]] bool isOn() const override;
    void turnOn() override;
    void turnOff() override;

protected:
    virtual void putPixel(Pixel pixel, uint8_t x, uint8_t y);

    bool on;
    uint8_t x;
    uint8_t y;
};

#endif // LCD_H