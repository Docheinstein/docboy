
#include "serial.h"

SerialLink::SerialLink() : master(), slave() {

}

SerialLink::~SerialLink() {

}

void SerialLink::tick() {
    if (master && slave) {
        uint8_t masterData = master->serialRead();
        uint8_t slaveData = slave->serialRead();
        master->serialWrite(slaveData);
        slave->serialWrite(masterData);
    }
}

void SerialLink::setMaster(ISerialEndpoint *e) {
    master = e;
}

void SerialLink::setSlave(ISerialEndpoint *e) {
    slave = e;
}

uint8_t ISerialEndpoint::serialRead() {
    return 0xFF;
}

ISerialEndpoint::~ISerialEndpoint() {

}

void SerialBufferEndpoint::serialWrite(uint8_t data) {
    buffer.push_back(data);
}

const std::vector<uint8_t> & SerialBufferEndpoint::getData() const {
    return buffer;
}

void SerialBufferEndpoint::clearData() {
    buffer.clear();
}

SerialBufferEndpoint::SerialBufferEndpoint() {

}

SerialBufferEndpoint::~SerialBufferEndpoint() {

}
