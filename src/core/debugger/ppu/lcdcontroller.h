#ifndef DEBUGGERLCDCONTROLLER_H
#define DEBUGGERLCDCONTROLLER_H

#include "core/ppu/lcdcontroller.h"
#include "core/debugger/io/lcd.h"

class DebuggableLCDController : public LCDController, public ILCDIODebug {
public:
    explicit DebuggableLCDController(IDMA &dma);

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

    void setObserver(Observer *o) override;

private:
    Observer *observer;
};
#endif // DEBUGGERLCDCONTROLLER_H