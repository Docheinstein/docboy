#ifndef LCD_H
#define LCD_H

#include <cstdint>

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

protected:
    virtual void putPixel(Pixel pixel, uint8_t x, uint8_t y) = 0;
};

class LCD : public virtual ILCD {
public:
    void pushPixel(Pixel pixel) override;

protected:
    uint8_t x;
    uint8_t y;
};

#endif // LCD_H