#ifndef DEBUGGERSERIALPORT_H
#define DEBUGGERSERIALPORT_H

#include "core/debugger/io/serial.h"
#include "core/serial/port.h"

class DebuggableSerialPort : public SerialPort, public ISerialIODebug {
public:
    explicit DebuggableSerialPort(IInterruptsIO& interrupts);

    [[nodiscard]] uint8_t readSB() const override;
    void writeSB(uint8_t value) override;

    [[nodiscard]] uint8_t readSC() const override;
    void writeSC(uint8_t value) override;

    void setObserver(Observer* o) override;

private:
    Observer* observer;
};

#endif // DEBUGGERSERIALPORT_H