#include "docboy/serial/link.h"
#include "docboy/serial/endpoint.h"

SerialLink::Plug::Plug(SerialLink& link) :
    link {link} {
}

SerialLink& SerialLink::Plug::attach(ISerialEndpoint& e) {
    endpoint = &e;
    return link;
}

void SerialLink::Plug::detach() {
    endpoint = nullptr;
}

void SerialLink::tick() const {
    if (plug1.endpoint) {
        plug1.endpoint->serial_write(plug2.endpoint ? plug2.endpoint->serial_read() : 0xFF);
    }
    if (plug2.endpoint) {
        plug2.endpoint->serial_write(plug1.endpoint ? plug1.endpoint->serial_read() : 0xFF);
    }
}
