#ifndef DEBUGGERFRAMEBUFFERLCD_H
#define DEBUGGERFRAMEBUFFERLCD_H

#include "lcd.h"
#include "core/lcd/framebufferlcd.h"
#include "core/debugger/io/lcd.h"

class DebuggableFrameBufferLCD : public FrameBufferLCD, public ILCDIODebug, public ILCDDebug {
public:
    ~DebuggableFrameBufferLCD() override = default;

    State getState() override;

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

#endif // DEBUGGERFRAMEBUFFERLCD_H