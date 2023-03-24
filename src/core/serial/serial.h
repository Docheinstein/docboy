#ifndef SERIAL_H
#define SERIAL_H

#include "core/io/serial.h"
#include "core/io/interrupts.h"

class SerialIO : public ISerialIO {
public:
    explicit SerialIO(IInterruptsIO &interrupts);

    [[nodiscard]] uint8_t readSB() const override;
    void writeSB(uint8_t value) override;

    [[nodiscard]] uint8_t readSC() const override;
    void writeSC(uint8_t value) override;

private:
    IInterruptsIO &interrupts;

    uint8_t SB;
    uint8_t SC;
};

#endif // SERIAL_H