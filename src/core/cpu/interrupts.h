#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "core/io/interrupts.h"

class Interrupts : public IInterruptsIO {
public:
    [[nodiscard]] uint8_t readIF() const override;
    void writeIF(uint8_t value) override;

    [[nodiscard]] uint8_t readIE() const override;
    void writeIE(uint8_t value) override;

private:
    uint8_t IF;
    uint8_t IE;
};

#endif // INTERRUPTS_H