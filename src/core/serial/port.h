#ifndef PORT_H
#define PORT_H

#include <memory>
#include "endpoint.h"
#include "link.h"
#include "core/io/serial.h"

class SerialLink;
class IInterruptsIO;

class ISerialPort {
public:
    virtual void attachSerialLink(SerialLink::Plug &plug) = 0;
    virtual void detachSerialLink() = 0;
};

class SerialPort : public ISerialPort, public ISerialEndpoint, public IClockable, public ISerialIO {
public:
    explicit SerialPort(IInterruptsIO &interrupts);

    // ISerialPort
    void attachSerialLink(SerialLink::Plug &plug) override;
    void detachSerialLink() override;

    // SerialEndpoint
    uint8_t serialRead() override;
    void serialWrite(uint8_t) override;

    // IClockable
    void tick() override;

    // ISerialIO
    [[nodiscard]] uint8_t readSB() const override;
    void writeSB(uint8_t value) override;

    [[nodiscard]] uint8_t readSC() const override;
    void writeSC(uint8_t value) override;

private:
    IInterruptsIO &interrupts;
    ISerialLink *link;

    uint8_t SB;
    uint8_t SC;
};

#endif // PORT_H