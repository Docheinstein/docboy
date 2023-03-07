#include "oam.h"

OAM::OAM() : MemoryImpl(MemoryMap::OAM::SIZE) {

}

uint8_t OAM::read(uint16_t index) const {
    return MemoryImpl::read(index);
}

void OAM::write(uint16_t index, uint8_t value) {
    MemoryImpl::write(index, value);
}
