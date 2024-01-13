#ifndef MMU_H
#define MMU_H

#include "docboy/bootrom/macros.h"
#include "docboy/bus/bus.h"
#include "docboy/memory/fwd/bytefwd.h"
#include "docboy/shared//macros.h"
#include "utils/macros.h"

#ifdef ENABLE_BOOTROM
#include "docboy/bootrom/fwd/bootromfwd.h"
#endif

class ExtBus;
class CpuBus;
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

    Mmu(IF_BOOTROM(BootRom& bootRom COMMA) ExtBus& extBus, CpuBus& cpuBus, VramBus& vramBus, OamBus& oamBus);

    template <Device::Type>
    void readRequest(uint16_t address);

    template <Device::Type>
    uint8_t flushReadRequest();

    template <Device::Type>
    void writeRequest(uint16_t address);

    template <Device::Type>
    void flushWriteRequest(uint16_t value);

    IF_BOOTROM(void lockBootRom());

    void saveState(Parcel& parcel) const;
    void loadState(Parcel& parcel);

private:
    struct BusAccess {
        IBus* bus {};

        struct {
            void (*readRequest)(IBus*, uint16_t) {};
            uint8_t (*flushReadRequest)(IBus*) {};
            void (*writeRequest)(IBus*, uint16_t) {};
            void (*flushWriteRequest)(IBus*, uint8_t) {};

#ifdef ENABLE_DEBUGGER
            uint8_t (*readBus)(const IBus*, uint16_t) {};
#endif
        } vtable;

        template <typename Bus, Device::Type Dev>
        static void readRequest(IBus* bus, uint16_t address);

        template <typename Bus, Device::Type Dev>
        static uint8_t flushReadRequest(IBus* bus);

        template <typename Bus, Device::Type Dev>
        static void writeRequest(IBus* bus, uint16_t address);

        template <typename Bus, Device::Type Dev>
        static void flushWriteRequest(IBus* bus, uint8_t value);

#ifdef ENABLE_DEBUGGER
        template <typename Bus>
        static uint8_t readBus(const IBus* bus, uint16_t address);
#endif

        void readRequest(uint16_t);
        uint8_t flushReadRequest();
        void writeRequest(uint16_t);
        void flushWriteRequest(uint8_t);

#ifdef ENABLE_DEBUGGER
        [[nodiscard]] uint8_t readBus(uint16_t) const;
#endif
    };

    static constexpr uint8_t NUM_DEVICES = 2;

    template <Device::Type Dev, typename Bus>
    static BusAccess makeAccessors(Bus* bus);

    IF_BOOTROM(BootRom& bootRom);

    ExtBus& extBus;
    CpuBus& cpuBus;
    VramBus& vramBus;
    OamBus& oamBus;

    BusAccess* requests[NUM_DEVICES] {};
    BusAccess busAccessors[NUM_DEVICES][UINT16_MAX + 1] {};
};

template <Device::Type>
class MmuView {
public:
    MmuView(Mmu& mmu);

    void readRequest(uint16_t address);
    uint8_t flushReadRequest();
    void writeRequest(uint16_t address);
    void flushWriteRequest(uint16_t value);

private:
    Mmu& mmu;
};

#include "mmu.tpp"

#endif // MMU_H