#include "docboy/mmu/mmu.h"

#include "docboy/boot/boot.h"
#include "docboy/bus/bus.h"
#include "docboy/bus/cpubus.h"
#include "docboy/bus/extbus.h"
#include "docboy/bus/oambus.h"
#include "docboy/bus/vrambus.h"
#include "docboy/bus/wrambus.h"

#include "utils/arrays.h"
#include "utils/parcel.h"

namespace {
constexpr uint8_t STATE_NO_REQUEST = 0;
constexpr uint8_t STATE_REQUEST_EXT_BUS = 1;
constexpr uint8_t STATE_REQUEST_CPU_BUS = 2;
constexpr uint8_t STATE_REQUEST_VRAM_BUS = 3;
constexpr uint8_t STATE_REQUEST_OAM_BUS = 4;
} // namespace

#ifdef ENABLE_BOOTROM
#ifdef ENABLE_CGB
Mmu::Mmu(BootRom& boot_rom, ExtBus& ext_bus, WramBus& wram_bus, CpuBus& cpu_bus, VramBus& vram_bus, OamBus& oam_bus) :
    boot_rom {boot_rom},
#else
Mmu::Mmu(BootRom& boot_rom, ExtBus& ext_bus, CpuBus& cpu_bus, VramBus& vram_bus, OamBus& oam_bus) :
    boot_rom {boot_rom},
#endif
#else
#ifdef ENABLE_CGB
Mmu::Mmu(ExtBus& ext_bus, WramBus& wram_bus, CpuBus& cpu_bus, VramBus& vram_bus, OamBus& oam_bus) :
#else
Mmu::Mmu(ExtBus& ext_bus, CpuBus& cpu_bus, VramBus& vram_bus, OamBus& oam_bus) :
#endif
#endif
    ext_bus {ext_bus},
#ifdef ENABLE_CGB
    wram_bus {wram_bus},
#endif
    cpu_bus {cpu_bus},
    vram_bus {vram_bus},
    oam_bus {oam_bus} {

#ifdef ENABLE_BOOTROM
    /* 0x0000 - 0x00FF */
    for (uint16_t i = Specs::MemoryLayout::BOOTROM::START; i <= Specs::MemoryLayout::BOOTROM::END; i++) {
        init_accessors(i, &cpu_bus);
    }
#else
    /* 0x0000 - 0x00FF */
    for (uint16_t i = Specs::MemoryLayout::BOOTROM::START; i <= Specs::MemoryLayout::BOOTROM::END; i++) {
        init_accessors(i, &ext_bus);
    }
#endif

    /* 0x0100 - 0x7FFF */
    for (uint16_t i = Specs::MemoryLayout::BOOTROM::END + 1; i <= Specs::MemoryLayout::ROM1::END; i++) {
        init_accessors(i, &ext_bus);
    }

    /* 0x8000 - 0x9FFF */
    for (uint16_t i = Specs::MemoryLayout::VRAM::START; i <= Specs::MemoryLayout::VRAM::END; i++) {
        init_accessors(i, &vram_bus);
    }

    /* 0xA000 - 0xBFFF */
    for (uint16_t i = Specs::MemoryLayout::RAM::START; i <= Specs::MemoryLayout::RAM::END; i++) {
        init_accessors(i, &ext_bus);
    }

    /* 0xC000 - 0xCFFF */
    for (uint16_t i = Specs::MemoryLayout::WRAM1::START; i <= Specs::MemoryLayout::WRAM1::END; i++) {
#ifdef ENABLE_CGB
        init_accessors(i, &wram_bus);
#else
        init_accessors(i, &ext_bus);
#endif
    }

    /* 0xD000 - 0xDFFF */
    for (uint16_t i = Specs::MemoryLayout::WRAM2::START; i <= Specs::MemoryLayout::WRAM2::END; i++) {
#ifdef ENABLE_CGB
        init_accessors(i, &wram_bus);
#else
        init_accessors(i, &ext_bus);
#endif
    }

    /* 0xE000 - 0xFDFF */
    for (uint16_t i = Specs::MemoryLayout::ECHO_RAM::START; i <= Specs::MemoryLayout::ECHO_RAM::END; i++) {
#ifdef ENABLE_CGB
        init_accessors(i, &wram_bus);
#else
        init_accessors(i, &ext_bus);
#endif
    }

    /* 0xFE00 - 0xFE9F */
    for (uint16_t i = Specs::MemoryLayout::OAM::START; i <= Specs::MemoryLayout::OAM::END; i++) {
        init_accessors(i, &oam_bus);
    }

    /* 0xFEA0 - 0xFEFF */
    for (uint16_t i = Specs::MemoryLayout::NOT_USABLE::START; i <= Specs::MemoryLayout::NOT_USABLE::END; i++) {
        init_accessors(i, &oam_bus);
    }

    /* 0xFF00 - 0xFFFF */
    for (uint16_t i = Specs::MemoryLayout::IO::START; i <= Specs::MemoryLayout::IO::END; i++) {
        init_accessors(i, &cpu_bus);
    }

    /* 0xFF80 - 0xFFFE */
    for (uint16_t i = Specs::MemoryLayout::HRAM::START; i <= Specs::MemoryLayout::HRAM::END; i++) {
        init_accessors(i, &cpu_bus);
    }

    /* 0xFFFF */
    init_accessors(Specs::MemoryLayout::IE, &cpu_bus);
}

template <Device::Type Dev, typename Bus>
Mmu::BusAccess Mmu::make_accessors(Bus* bus) {
#ifdef ENABLE_DEBUGGER
    return BusAccess {bus,
                      {BusAccess::read_request<Bus, Dev>, BusAccess::flush_read_request<Bus, Dev>,
                       BusAccess::write_request<Bus, Dev>, BusAccess::flush_write_request<Bus, Dev>,
                       BusAccess::read_bus<Bus>}};
#else
    return BusAccess {bus,
                      {BusAccess::read_request<Bus, Dev>, BusAccess::flush_read_request<Bus, Dev>,
                       BusAccess::write_request<Bus, Dev>, BusAccess::flush_write_request<Bus, Dev>}};
#endif
}

template <typename Bus>
void Mmu::init_accessors(uint16_t address, Bus* bus) {
    bus_accessors[Device::Cpu][address] = make_accessors<Device::Cpu>(bus);
    bus_accessors[Device::Dma][address] = make_accessors<Device::Dma>(bus);
#ifdef ENABLE_CGB
    bus_accessors[Device::Hdma][address] = make_accessors<Device::Hdma>(bus);
#endif
}

#ifdef ENABLE_BOOTROM
void Mmu::lock_boot_rom() {
    ASSERT(!boot_rom_locked);
    boot_rom_locked = true;
    // Remap Boot Rom address range [0x0000 - 0x00FF] to EXT Bus.
    for (uint16_t i = Specs::MemoryLayout::BOOTROM::START; i <= Specs::MemoryLayout::BOOTROM::END; i++) {
        init_accessors(i, &ext_bus);
    }
}
#endif

void Mmu::save_state(Parcel& parcel) const {
    const auto save_request = [this, &parcel](const BusAccess* request) {
        parcel.write_uint8([this, request] {
            if (!request) {
                return STATE_NO_REQUEST;
            }

            const IBus* bus = request->bus;
            if (bus == &ext_bus) {
                return STATE_REQUEST_EXT_BUS;
            }
            if (bus == &cpu_bus) {
                return STATE_REQUEST_CPU_BUS;
            }
            if (bus == &vram_bus) {
                return STATE_REQUEST_VRAM_BUS;
            }
            if (bus == &oam_bus) {
                return STATE_REQUEST_OAM_BUS;
            }

            ASSERT_NO_ENTRY();
            return (uint8_t)0;
        }());
    };

    for (uint32_t i = 0; i < array_size(requests); i++) {
        save_request(requests[i]);
    }

#ifdef ENABLE_BOOTROM
    parcel.write_bool(boot_rom_locked);
#endif
}

void Mmu::load_state(Parcel& parcel) {
    const auto load_request = [this, &parcel](Device::Type device, BusAccess*& request) {
        request = [this, device](uint8_t value) -> BusAccess* {
            if (value == STATE_NO_REQUEST) {
                return nullptr;
            }
            if (value == STATE_REQUEST_EXT_BUS) {
                return &bus_accessors[device][Specs::MemoryLayout::WRAM1::START];
            }
            if (value == STATE_REQUEST_CPU_BUS) {
                return &bus_accessors[device][Specs::MemoryLayout::HRAM::START];
            }
            if (value == STATE_REQUEST_VRAM_BUS) {
                return &bus_accessors[device][Specs::MemoryLayout::VRAM::START];
            }
            if (value == STATE_REQUEST_OAM_BUS) {
                return &bus_accessors[device][Specs::MemoryLayout::OAM::START];
            }

            ASSERT_NO_ENTRY();
            return nullptr;
        }(parcel.read_uint8());
    };

    for (uint32_t i = 0; i < array_size(requests); i++) {
        load_request(i, requests[i]);
    }

#ifdef ENABLE_BOOTROM
    boot_rom_locked = parcel.read_bool();
    if (boot_rom_locked) {
        for (uint16_t i = Specs::MemoryLayout::BOOTROM::START; i <= Specs::MemoryLayout::BOOTROM::END; i++) {
            init_accessors(i, &cpu_bus);
        }
    } else {
        for (uint16_t i = Specs::MemoryLayout::BOOTROM::START; i <= Specs::MemoryLayout::BOOTROM::END; i++) {
            init_accessors(i, &ext_bus);
        }
    }
#endif
}
void Mmu::reset() {
#ifdef ENABLE_BOOTROM
    boot_rom_locked = false;
    for (uint16_t i = Specs::MemoryLayout::BOOTROM::START; i <= Specs::MemoryLayout::BOOTROM::END; i++) {
        init_accessors(i, &cpu_bus);
    }
#else
    for (uint16_t i = Specs::MemoryLayout::BOOTROM::START; i <= Specs::MemoryLayout::BOOTROM::END; i++) {
        init_accessors(i, &ext_bus);
    }
#endif

    for (uint32_t i = 0; i < array_size(requests); i++) {
        requests[i] = nullptr;
    }
}
