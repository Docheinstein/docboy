#include "io.h"

IO::IO() {

}

IO::~IO() {

}

uint8_t IO::read(size_t index) const {
    // DEBUG
    if (index == 0x44)
        return 0x90;
    return memory[index];
}

void IO::write(size_t index, uint8_t value) {
    memory[index] = value;
}

void IO::reset() {
    memset(memory, 0, 256);
}

uint8_t IO::readSB() const {
    return read(0x01);
}

uint8_t IO::readSC() const {
    return read(0x02);
}

void IO::writeSB(uint8_t value) {
    write(0x01, value);
}

void IO::writeSC(uint8_t value) {
    write(0x02, value);
}

