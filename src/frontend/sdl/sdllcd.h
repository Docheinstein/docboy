#ifndef SDLLCD_H
#define SDLLCD_H

#include "core/ppu/lcd.h"
#include "core/definitions.h"
#include "core/components.h"

class SDLLCD : public LCDImpl {
public:
    SDLLCD();
    ~SDLLCD() override = default;

    [[nodiscard]] bool isOn() const override;
    void turnOn() override;
    void turnOff() override;

    uint32_t *getFrameBuffer();

protected:
    void putPixel(Pixel pixel, uint8_t x, uint8_t y) override;

    bool on;
    uint32_t pixels[Specs::Display::WIDTH * Specs::Display::HEIGHT];
};

#endif // SDLLCD_H