#include "mmu.h"
#include "docboy/boot/boot.h"
#include "docboy/bus/bus.h"
#include "docboy/bus/cpubus.h"
#include "docboy/bus/extbus.h"
#include "docboy/bus/oambus.h"
#include "docboy/bus/vrambus.h"
#include "utils/arrays.h"
#include "utils/parcel.h"

static constexpr uint8_t STATE_NO_REQUEST = 0;
static constexpr uint8_t STATE_REQUEST_EXT_BUS = 1;
static constexpr uint8_t STATE_REQUEST_CPU_BUS = 2;
static constexpr uint8_t STATE_REQUEST_VRAM_BUS = 3;
static constexpr uint8_t STATE_REQUEST_OAM_BUS = 4;

Mmu::Mmu(IF_BOOTROM(BootRom& bootRom COMMA) ExtBus& extBus, CpuBus& cpuBus, VramBus& vramBus, OamBus& oamBus) :
    IF_BOOTROM(bootRom(bootRom) COMMA) extBus(extBus),
    cpuBus(cpuBus),
    vramBus(vramBus),
    oamBus(oamBus) {

    const auto makeAccessorsForDevices = [this](uint16_t i, auto* bus) {
        busAccessors[Device::Cpu][i] = makeAccessors<Device::Cpu>(bus);
        busAccessors[Device::Dma][i] = makeAccessors<Device::Dma>(bus);
    };

#ifdef ENABLE_BOOTROM
    /* 0x0000 - 0x00FF */
    for (uint16_t i = Specs::MemoryLayout::BOOTROM::START; i <= Specs::MemoryLayout::BOOTROM::END; i++) {
        makeAccessorsForDevices(i, &cpuBus);
    }
#else
    /* 0x0000 - 0x00FF */
    for (uint16_t i = Specs::MemoryLayout::BOOTROM::START; i <= Specs::MemoryLayout::BOOTROM::END; i++) {
        makeAccessorsForDevices(i, &extBus);
    }
#endif

    /* 0x0100 - 0x7FFF */
    for (uint16_t i = Specs::MemoryLayout::BOOTROM::END + 1; i <= Specs::MemoryLayout::ROM1::END; i++) {
        makeAccessorsForDevices(i, &extBus);
    }

    /* 0x8000 - 0x9FFF */
    for (uint16_t i = Specs::MemoryLayout::VRAM::START; i <= Specs::MemoryLayout::VRAM::END; i++) {
        makeAccessorsForDevices(i, &vramBus);
    }

    /* 0xA000 - 0xBFFF */
    for (uint16_t i = Specs::MemoryLayout::RAM::START; i <= Specs::MemoryLayout::RAM::END; i++) {
        makeAccessorsForDevices(i, &extBus);
    }

    /* 0xC000 - 0xCFFF */
    for (uint16_t i = Specs::MemoryLayout::WRAM1::START; i <= Specs::MemoryLayout::WRAM1::END; i++) {
        makeAccessorsForDevices(i, &extBus);
    }

    /* 0xD000 - 0xDFFF */
    for (uint16_t i = Specs::MemoryLayout::WRAM2::START; i <= Specs::MemoryLayout::WRAM2::END; i++) {
        makeAccessorsForDevices(i, &extBus);
    }

    /* 0xE000 - 0xFDFF */
    for (uint16_t i = Specs::MemoryLayout::ECHO_RAM::START; i <= Specs::MemoryLayout::ECHO_RAM::END; i++) {
        makeAccessorsForDevices(i, &extBus);
    }

    /* 0xFE00 - 0xFE9F */
    for (uint16_t i = Specs::MemoryLayout::OAM::START; i <= Specs::MemoryLayout::OAM::END; i++) {
        makeAccessorsForDevices(i, &oamBus);
    }

    /* 0xFEA0 - 0xFEFF */
    for (uint16_t i = Specs::MemoryLayout::NOT_USABLE::START; i <= Specs::MemoryLayout::NOT_USABLE::END; i++) {
        makeAccessorsForDevices(i, &oamBus);
    }

    /* 0xFF00 - 0xFFFF */
    for (uint16_t i = Specs::MemoryLayout::IO::START; i <= Specs::MemoryLayout::IO::END; i++) {
        makeAccessorsForDevices(i, &cpuBus);
    }

    /* 0xFF80 - 0xFFFE */
    for (uint16_t i = Specs::MemoryLayout::HRAM::START; i <= Specs::MemoryLayout::HRAM::END; i++) {
        makeAccessorsForDevices(i, &cpuBus);
    }

    /* 0xFFFF */
    makeAccessorsForDevices(Specs::MemoryLayout::IE, &cpuBus);
}

template <Device::Type Dev, typename Bus>
Mmu::BusAccess Mmu::makeAccessors(Bus* bus) {
#ifdef ENABLE_DEBUGGER
    return BusAccess {bus,
                      BusAccess::readRequest<Bus, Dev>,
                      BusAccess::flushReadRequest<Bus, Dev>,
                      BusAccess::writeRequest<Bus, Dev>,
                      BusAccess::flushWriteRequest<Bus, Dev>,
                      BusAccess::readBus<Bus>};
#else
    return BusAccess {bus, BusAccess::readRequest<Bus, Dev>, BusAccess::flushReadRequest<Bus, Dev>,
                      BusAccess::writeRequest<Bus, Dev>, BusAccess::flushWriteRequest<Bus, Dev>};
#endif
}

#ifdef ENABLE_BOOTROM
inline void Mmu::lockBootRom() {
    // Remap Boot Rom address range [0x0000 - 0x00FF] to EXT Bus.
    for (uint16_t i = Specs::MemoryLayout::BOOTROM::START; i <= Specs::MemoryLayout::BOOTROM::END; i++) {
        busAccessors[i] = {&extBus, &ExtBus::read, &ExtBus::write};
    }
    // TODO: tecnically this is part of the state and should parcelized
}
#endif

void Mmu::saveState(Parcel& parcel) const {
    const auto saveRequest = [this, &parcel](const BusAccess* request) {
        parcel.writeUInt8([this, request] {
            if (!request)
                return STATE_NO_REQUEST;

            const IBus* bus = request->bus;
            if (bus == &extBus)
                return STATE_REQUEST_EXT_BUS;
            if (bus == &cpuBus)
                return STATE_REQUEST_CPU_BUS;
            if (bus == &vramBus)
                return STATE_REQUEST_VRAM_BUS;
            if (bus == &oamBus)
                return STATE_REQUEST_OAM_BUS;

            checkNoEntry();
            return (uint8_t)0;
        }());
    };

    for (uint32_t i = 0; i < array_size(requests); i++)
        saveRequest(requests[i]);
}

void Mmu::loadState(Parcel& parcel) {
    const auto loadRequest = [this, &parcel](Device::Type device, BusAccess*& request) {
        request = [this, device](uint8_t value) -> BusAccess* {
            if (value == STATE_NO_REQUEST)
                return nullptr;
            if (value == STATE_REQUEST_EXT_BUS)
                return &busAccessors[device][Specs::MemoryLayout::WRAM1::START];
            if (value == STATE_REQUEST_CPU_BUS)
                return &busAccessors[device][Specs::MemoryLayout::HRAM::START];
            if (value == STATE_REQUEST_VRAM_BUS)
                return &busAccessors[device][Specs::MemoryLayout::VRAM::START];
            if (value == STATE_REQUEST_OAM_BUS)
                return &busAccessors[device][Specs::MemoryLayout::OAM::START];

            checkNoEntry();
            return nullptr;
        }(parcel.readUInt8());
    };

    for (uint32_t i = 0; i < array_size(requests); i++)
        loadRequest(i, requests[i]);
}