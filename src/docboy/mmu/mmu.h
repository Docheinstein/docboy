#ifndef MMU_H
#define MMU_H

#include "docboy/bootrom/macros.h"
#include "docboy/bus/bus.h"
#include "docboy/debugger/macros.h"
#include "docboy/memory/fwd/bytefwd.h"
#include "utils/macros.h"

#ifdef ENABLE_BOOTROM
#include "docboy/bootrom/fwd/bootromfwd.h"
#endif

class ExtBus;
class CpuBus;
class OamBus;
class VramBus;
class Parcel;

struct MmuDevice {
    using Type = uint8_t;
    static constexpr Type Cpu = 0;
    static constexpr Type Dma = 1;
};

class MmuIO;

template <MmuDevice::Type>
class MmuView {
public:
    explicit MmuView(MmuIO& mmu, uint8_t* latch);

    void requestRead(uint16_t address);
    void requestWrite(uint16_t address, uint8_t value);

private:
    MmuIO& mmu;
    uint8_t* latch;
};

template <MmuDevice::Type d>
class MmuSocket {
public:
    explicit MmuSocket(MmuIO& mmu, uint8_t*& latchRef);

    MmuView<d> attach(uint8_t* latch);

private:
    MmuIO& mmu;
    uint8_t*& latchRef;
};

class MmuIO {
    DEBUGGABLE_CLASS()

public:
    friend class MmuView<MmuDevice::Cpu>;
    friend class MmuView<MmuDevice::Dma>;

    using Device = MmuDevice;

    MmuIO(IF_BOOTROM(BootRom& bootRom COMMA) ExtBus& extBus, CpuBus& cpuBus, VramBus& vramBus, OamBus& oamBus);

    template <MmuDevice::Type d>
    MmuSocket<d> socket();

    void saveState(Parcel& parcel) const;
    void loadState(Parcel& parcel);

protected:
    struct BusLane {
        enum class State : uint8_t { None, Read, Write };
        State state {State::None};
        uint16_t address {};
        uint8_t* data {};
    };

    struct BusAccess {
        IBus* bus {};
        uint8_t (*read)(const IBus*, uint16_t) {};
        void (*write)(IBus*, uint16_t, uint8_t) {};
    };

    BusLane lanes[2] {};
    BusAccess busAccessors[UINT16_MAX + 1] {};

    IF_BOOTROM(BootRom& bootRom);

    ExtBus& extBus;
    CpuBus& cpuBus;
    VramBus& vramBus;
    OamBus& oamBus;

private:
    template <Device::Type>
    void requestRead(uint16_t address);

    template <Device::Type>
    void requestWrite(uint16_t address);

    [[nodiscard]] IBus* busAt(uint16_t address) const;
};

class Mmu : public MmuIO {
    DEBUGGABLE_CLASS()

public:
    Mmu(IF_BOOTROM(BootRom& bootRom COMMA) ExtBus& extBus, CpuBus& cpuBus, VramBus& vramBus, OamBus& oamBus);

    void tick_t0();
    void tick_t1();
    void tick_t2();
    void tick_t3();

    IF_BOOTROM(void lockBootRom());

private:
    void handleReadRequest(BusLane& lane);
    void handleWriteRequest(BusLane& lane);
};

#include "mmu.hpp"

#endif // MMU_H