#ifndef PORT_H
#define PORT_H

#include "docboy/serial/endpoint.h"
#include "docboy/serial/link.h"
#include "docboy/serial/serial.h"

class Interrupts;

class SerialPort : public Serial, public ISerialEndpoint {
public:
    explicit SerialPort(Interrupts& interrupts);

    void attach(SerialLink::Plug& plug);
    void detach();

    uint8_t serial_read() override;
    void serial_write(uint8_t) override;

    void tick();

private:
    Interrupts& interrupts;

    SerialLink* link {};
};

#endif // PORT_H