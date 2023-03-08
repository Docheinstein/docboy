#ifndef PORT_H
#define PORT_H

#include <memory>
#include "endpoint.h"
#include "link.h"

class SerialLink;
class IBus;

class SerialPort : public SerialEndpoint {
public:
    explicit SerialPort(IBus &bus);

    void attachSerialLink(SerialLink::Plug &plug);
    void detachSerialLink();

    uint8_t serialRead() override;
    void serialWrite(uint8_t) override;

    void tick();

private:
    IBus &bus;
    ISerialLink *link;
};

#endif // PORT_H