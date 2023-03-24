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
};

class LCD : public ILCD, public ILCDIO {
public:
    LCD();
    ~LCD() override = default;

    void pushPixel(Pixel pixel) override;

    [[nodiscard]] uint8_t readLCDC() const override;
    void writeLCDC(uint8_t value) override;

    [[nodiscard]] uint8_t readSTAT() const override;
    void writeSTAT(uint8_t value) override;

    [[nodiscard]] uint8_t readSCY() const override;
    void writeSCY(uint8_t value) override;

    [[nodiscard]] uint8_t readSCX() const override;
    void writeSCX(uint8_t value) override;

    [[nodiscard]] uint8_t readLY() const override;
    void writeLY(uint8_t value) override;

    [[nodiscard]] uint8_t readLYC() const override;
    void writeLYC(uint8_t value) override;

    [[nodiscard]] uint8_t readDMA() const override;
    void writeDMA(uint8_t value) override;

    [[nodiscard]] uint8_t readBGP() const override;
    void writeBGP(uint8_t value) override;

    [[nodiscard]] uint8_t readOBP0() const override;
    void writeOBP0(uint8_t value) override;

    [[nodiscard]] uint8_t readOBP1() const override;
    void writeOBP1(uint8_t value) override;

    [[nodiscard]] uint8_t readWY() const override;
    void writeWY(uint8_t value) override;

    [[nodiscard]] uint8_t readWX() const override;
    void writeWX(uint8_t value) override;

protected:
    virtual void putPixel(Pixel pixel, uint8_t x, uint8_t y);

    void reset();

    uint8_t LCDC;
    uint8_t STAT;
    uint8_t SCY;
    uint8_t SCX;
    uint8_t LY;
    uint8_t LYC;
    uint8_t DMA;
    uint8_t BGP;
    uint8_t OBP0;
    uint8_t OBP1;
    uint8_t WY;
    uint8_t WX;

    uint8_t x;
    uint8_t y;
};

#endif // LCD_H