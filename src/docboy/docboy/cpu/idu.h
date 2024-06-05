#ifndef IDU_H
#define IDU_H

#include <cstdint>

#include "docboy/bus/oambus.h"
#include "docboy/common/specs.h"

class Idu {
public:
    DEBUGGABLE_CLASS()

    explicit Idu(OamBus& oam_bus) :
        oam_bus {oam_bus} {
    }

    void tick_t1() {
        // Eventually clear any previous write request.
        oam_bus.clear_write_request<Device::Cpu>();
    }

    void increment(uint16_t& data) {
        increment<1>(data);
    }

    void decrement(uint16_t& data) {
        increment<-1>(data);
    }

    template <int8_t inc>
    void increment(uint16_t& data) {
        // Since IDU is directly connected to the address bus, any increment/decrement
        // operation put the word the IDU is managing to the address bus.
        // This might lead to the OAM Bug if such address is in the FE00-FEFF
        // range and the PPU is reading from OAM at that moment (DMG only).
        // [blargg/oam_bug/2-causes]
        if (data >= Specs::MemoryLayout::OAM::START && data <= Specs::MemoryLayout::NOT_USABLE::END) {
            oam_bus.write_request<Device::Cpu>(data);
        }

        data += inc;
    }

private:
    OamBus& oam_bus;
};

#endif // IDU_H