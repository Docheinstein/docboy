#ifndef IDU_HPP
#define IDU_HPP

#include "docboy/bus/oambus.h"
#include "docboy/shared/macros.h"
#include "docboy/shared/specs.h"
#include <cstdint>

class Idu {
public:
    DEBUGGABLE_CLASS()

    explicit Idu(OamBus& oamBus) :
        oamBus {oamBus} {
    }

    void tick_t1() {
        // Eventually clear any previous write request.
        oamBus.clearWriteRequest<Device::Cpu>();
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
        if (data >= Specs::MemoryLayout::OAM::START && data <= Specs::MemoryLayout::NOT_USABLE::END)
            oamBus.writeRequest<Device::Cpu>(data);

        data += inc;
    }

private:
    OamBus& oamBus;
};

#endif // IDU_HPP