#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "core/io/interrupts.h"
#include "core/state/processor.h"

class Interrupts : public IInterruptsIO, public IStateProcessor {
public:
    void loadState(IReadableState& state) override;
    void saveState(IWritableState& state) override;

    [[nodiscard]] uint8_t readIF() const override;
    void writeIF(uint8_t value) override;

    [[nodiscard]] uint8_t readIE() const override;
    void writeIE(uint8_t value) override;

private:
    uint8_t IF;
    uint8_t IE;
};

#endif // INTERRUPTS_H