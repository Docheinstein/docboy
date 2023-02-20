#include "serial.h"

SerialEndpoint::~SerialEndpoint() = default;

SerialLink::SerialLink() : master(), slave() {}

SerialLink::~SerialLink() = default;

void SerialLink::tick() {
    if (master && slave) {
        uint8_t masterData = master->serialRead();
        uint8_t slaveData = slave->serialRead();
        master->serialWrite(slaveData);
        slave->serialWrite(masterData);
    }
}

void SerialLink::setMaster(SerialEndpoint *e) {
    master = e;
}

void SerialLink::setSlave(SerialEndpoint *e) {
    slave = e;
}

SerialLink::SerialLink(SerialEndpoint *master, SerialEndpoint *slave) {
    this->master = master;
    this->slave = slave;
}

uint8_t SerialEndpoint::serialRead() {
    return 0xFF;
}

void SerialEndpoint::serialWrite(uint8_t) {

}
