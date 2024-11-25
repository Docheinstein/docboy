#ifndef MMU_H
#define MMU_H

#include "docboy/bus/bus.h"
#include "docboy/common/macros.h"
#include "docboy/memory/fwd/cellfwd.h"

#ifdef ENABLE_BOOTROM
#include "docboy/bootrom/fwd/bootromfwd.h"
#endif

class ExtBus;
class CpuBus;
class WramBus;
class OamBus;
class VramBus;
class Parcel;

template <Device::Type>
class MmuView;

class Mmu {
    DEBUGGABLE_CLASS()
    TESTABLE_CLASS()

public:
    template <Device::Type Dev>
    using View = MmuView<Dev>;

#ifdef ENABLE_BOOTROM
#ifdef ENABLE_CGB
    Mmu(BootRom& boot_rom, ExtBus& ext_bus, WramBus& wram_bus, CpuBus& cpu_bus, VramBus& vram_bus, OamBus& oam_bus);
#else
    Mmu(BootRom& boot_rom, ExtBus& ext_bus, CpuBus& cpu_bus, VramBus& vram_bus, OamBus& oam_bus);
#endif
#else
#ifdef ENABLE_CGB
    Mmu(ExtBus& ext_bus, WramBus& wram_bus, CpuBus& cpu_bus, VramBus& vram_bus, OamBus& oam_bus);
#else
    Mmu(ExtBus& ext_bus, CpuBus& cpu_bus, VramBus& vram_bus, OamBus& oam_bus);
#endif
#endif

    template <Device::Type>
    void read_request(uint16_t address);

    template <Device::Type>
    uint8_t flush_read_request();

    template <Device::Type>
    void write_request(uint16_t address);

    template <Device::Type>
    void flush_write_request(uint16_t value);

#ifdef ENABLE_BOOTROM
    void lock_boot_rom();
#endif

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

private:
    struct BusAccess {
        IBus* bus {};

        struct {
            void (*read_request)(IBus*, uint16_t) {};
            uint8_t (*flush_read_request)(IBus*) {};
            void (*write_request)(IBus*, uint16_t) {};
            void (*flush_write_request)(IBus*, uint8_t) {};

#ifdef ENABLE_DEBUGGER
            uint8_t (*read_bus)(const IBus*, uint16_t) {};
#endif
        } vtable;

        template <typename Bus, Device::Type Dev>
        static void read_request(IBus* bus, uint16_t address);

        template <typename Bus, Device::Type Dev>
        static uint8_t flush_read_request(IBus* bus);

        template <typename Bus, Device::Type Dev>
        static void write_request(IBus* bus, uint16_t address);

        template <typename Bus, Device::Type Dev>
        static void flush_write_request(IBus* bus, uint8_t value);

#ifdef ENABLE_DEBUGGER
        template <typename Bus>
        static uint8_t read_bus(const IBus* bus, uint16_t address);
#endif

        void read_request(uint16_t);
        uint8_t flush_read_request();
        void write_request(uint16_t);
        void flush_write_request(uint8_t);

#ifdef ENABLE_DEBUGGER
        uint8_t read_bus(uint16_t) const;
#endif
    };

#ifdef ENABLE_CGB
    static constexpr uint8_t NUM_DEVICES = 3 /* Cpu, Dma, Hdma */;
#else
    static constexpr uint8_t NUM_DEVICES = 2 /* Cpu, Dma */;
#endif

    template <Device::Type Dev, typename Bus>
    static BusAccess make_accessors(Bus* bus);

    template <typename Bus>
    void init_accessors(uint16_t address, Bus* bus);

#ifdef ENABLE_BOOTROM
    BootRom& boot_rom;
    bool boot_rom_locked {};
#endif

    ExtBus& ext_bus;
#ifdef ENABLE_CGB
    WramBus& wram_bus;
#endif
    CpuBus& cpu_bus;
    VramBus& vram_bus;
    OamBus& oam_bus;

    BusAccess* requests[NUM_DEVICES] {};
    BusAccess bus_accessors[NUM_DEVICES][UINT16_MAX + 1] {};
};

template <Device::Type>
class MmuView {
public:
    MmuView(Mmu& mmu);

    void read_request(uint16_t address);
    uint8_t flush_read_request();
    void write_request(uint16_t address);
    void flush_write_request(uint16_t value);

private:
    Mmu& mmu;
};

#include "docboy/mmu/mmu.tpp"

#endif // MMU_H