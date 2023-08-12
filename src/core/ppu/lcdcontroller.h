#ifndef LCDCONTROLLER_H
#define LCDCONTROLLER_H

#include "core/io/lcd.h"
#include "core/state/processor.h"

class IDMA;

class LCDController : public ILCDIO, public IStateProcessor {
public:
    explicit LCDController(IDMA& dma);

    void loadState(IReadableState& state) override;
    void saveState(IWritableState& state) override;

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
    IDMA& dma;

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
};
#endif // LCDCONTROLLER_H