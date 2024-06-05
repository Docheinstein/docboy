#ifndef PORT_H
#define PORT_H

#include "docboy/serial/endpoint.h"
#include "docboy/serial/link.h"
#include "docboy/serial/serial.h"

class InterruptsIO;

class SerialPort : public SerialIO, public ISerialEndpoint {
public:
    explicit SerialPort(InterruptsIO& interrupts);

    void attach(SerialLink::Plug& plug);
    void detach();

    uint8_t serial_read() override;
    void serial_write(uint8_t) override;

    void tick();

private:
    InterruptsIO& interrupts;

    SerialLink* link {};
};

#endif // PORT_H