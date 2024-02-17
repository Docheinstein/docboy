#ifndef PORT_H
#define PORT_H

#include "endpoint.h"
#include "link.h"
#include "serial.h"

class InterruptsIO;

class SerialPort : public SerialIO, public ISerialEndpoint {
public:
    explicit SerialPort(InterruptsIO& interrupts);

    void attach(SerialLink::Plug& plug);
    void detach();

    uint8_t serialRead() override;
    void serialWrite(uint8_t) override;

    void tick();

private:
    InterruptsIO& interrupts;

    SerialLink* link {};
};

#endif // PORT_H