#ifndef DEBUGGERJOYPAD_H
#define DEBUGGERJOYPAD_H

#include "core/debugger/io/joypad.h"
#include "core/debugger/memory/memory.h"
#include "core/joypad/joypad.h"

class DebuggableJoypad : public IJoypadIODebug, public Joypad {
public:
    explicit DebuggableJoypad(IInterruptsIO& interrupts);

    [[nodiscard]] uint8_t readP1() const override;
    void writeP1(uint8_t value) override;

    void setObserver(Observer* o) override;

private:
    Observer* observer;
};
#endif // DEBUGGERJOYPAD_H