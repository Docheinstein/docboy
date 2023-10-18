#include "link.h"
#include "endpoint.h"

SerialLink::Plug::Plug(SerialLink& link) :
    link(link) {
}

SerialLink& SerialLink::Plug::attach(ISerialEndpoint& e) {
    endpoint = &e;
    return link;
}

void SerialLink::Plug::detach() {
    endpoint = nullptr;
}

void SerialLink::tick() const {
    if (plug1.endpoint)
        plug1.endpoint->serialWrite(plug2.endpoint ? plug2.endpoint->serialRead() : 0xFF);
    if (plug2.endpoint)
        plug2.endpoint->serialWrite(plug1.endpoint ? plug1.endpoint->serialRead() : 0xFF);
}
