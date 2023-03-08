#include "link.h"
#include "endpoint.h"
#include <cstdint>

SerialLink::SerialLink() : plug1(*this), plug2(*this) {

}


void SerialLink::tick() {
    if (plug1.endpoint && plug2.endpoint) {
        uint8_t data1 = plug1.endpoint->serialRead();
        uint8_t data2 = plug2.endpoint->serialRead();
        plug1.endpoint->serialWrite(data2);
        plug2.endpoint->serialWrite(data1);
    }
}

SerialLink::Plug::Plug(ISerialLink &link) :
    link(link), endpoint() {

}

ISerialLink &SerialLink::Plug::attach(SerialEndpoint *e) {
    endpoint = e;
    return link;
}

void SerialLink::Plug::detach() {
    endpoint = nullptr;
}
