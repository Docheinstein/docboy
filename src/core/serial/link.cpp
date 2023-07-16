#include "link.h"
#include "endpoint.h"
#include <cstdint>

SerialLink::SerialLink() :
    plug1(*this),
    plug2(*this) {
}

void SerialLink::tick() {
    uint8_t data1 = plug1.endpoint ? plug1.endpoint->serialRead() : 0xFF;
    uint8_t data2 = plug2.endpoint ? plug2.endpoint->serialRead() : 0xFF;
    if (plug1.endpoint)
        plug1.endpoint->serialWrite(data2);
    if (plug2.endpoint)
        plug2.endpoint->serialWrite(data1);
}

SerialLink::Plug::Plug(ISerialLink& link) :
    link(link),
    endpoint() {
}

ISerialLink& SerialLink::Plug::attach(ISerialEndpoint* e) {
    endpoint = e;
    return link;
}

void SerialLink::Plug::detach() {
    endpoint = nullptr;
}
