#ifndef DEBUGGERINTERRUPTS_H
#define DEBUGGERINTERRUPTS_H

#include "core/cpu/interrupts.h"
#include "core/debugger/io/interrupts.h"

class DebuggableInterrupts : public IInterruptsIODebug, public Interrupts {
public:
    [[nodiscard]] uint8_t readIF() const override;
    void writeIF(uint8_t value) override;

    [[nodiscard]] uint8_t readIE() const override;
    void writeIE(uint8_t value) override;

    void setObserver(Observer *observer) override;

private:
    Observer *observer;
};

#endif // DEBUGGERINTERRUPTS_H