#include "docboy/boot/boot.h"
#include "docboy/bus/bus.h"
#include "docboy/bus/cpubus.h"
#include "docboy/bus/extbus.h"
#include "docboy/bus/oambus.h"
#include "docboy/bus/vrambus.h"
#include "mmu.h"
#include "utils/arrays.h"
#include "utils/parcel.h"

template <MmuDevice::Type d>
MmuView<d>::MmuView(MmuIO& mmu, uint8_t* latch) :
    mmu(mmu),
    latch(latch) {
}

template <MmuDevice::Type d>
void MmuView<d>::requestRead(uint16_t address) {
    mmu.requestRead<d>(address);
}

template <MmuDevice::Type d>
void MmuView<d>::requestWrite(uint16_t address, uint8_t value) {
    *latch = value;
    mmu.requestWrite<d>(address);
}

template <MmuDevice::Type d>
MmuSocket<d>::MmuSocket(MmuIO& mmu, uint8_t*& latchRef) :
    mmu(mmu),
    latchRef(latchRef) {
}

template <MmuDevice::Type d>
MmuView<d> MmuSocket<d>::attach(uint8_t* latch) {
    latchRef = latch;
    return MmuView<d>(mmu, latch);
}

inline MmuIO::MmuIO(IF_BOOTROM(BootRom& bootRom COMMA) ExtBus& extBus, CpuBus& cpuBus, VramBus& vramBus,
                    OamBus& oamBus) :
    IF_BOOTROM(bootRom(bootRom) COMMA) extBus(extBus),
    cpuBus(cpuBus),
    vramBus(vramBus),
    oamBus(oamBus) {

#ifdef ENABLE_BOOTROM
    /* 0x0000 - 0x00FF */
    for (uint16_t i = Specs::MemoryLayout::BOOTROM::START; i <= Specs::MemoryLayout::BOOTROM::END; i++) {
        busAccessors[i] = {&cpuBus, &CpuBus::read, &CpuBus::write};
    }
#else
    /* 0x0000 - 0x00FF */
    for (uint16_t i = Specs::MemoryLayout::BOOTROM::START; i <= Specs::MemoryLayout::BOOTROM::END; i++) {
        busAccessors[i] = {&extBus, &ExtBus::read, &ExtBus::write};
    }
#endif

    /* 0x0100 - 0x7FFF */
    for (uint16_t i = Specs::MemoryLayout::BOOTROM::END + 1; i <= Specs::MemoryLayout::ROM1::END; i++) {
        busAccessors[i] = {&extBus, &ExtBus::read, &ExtBus::write};
    }

    /* 0x8000 - 0x9FFF */
    for (uint16_t i = Specs::MemoryLayout::VRAM::START; i <= Specs::MemoryLayout::VRAM::END; i++) {
        busAccessors[i] = {&vramBus, &VramBus::read, &VramBus::write};
    }

    /* 0xA000 - 0xBFFF */
    for (uint16_t i = Specs::MemoryLayout::RAM::START; i <= Specs::MemoryLayout::RAM::END; i++) {
        busAccessors[i] = {&extBus, &ExtBus::read, &ExtBus::write};
    }

    /* 0xC000 - 0xCFFF */
    for (uint16_t i = Specs::MemoryLayout::WRAM1::START; i <= Specs::MemoryLayout::WRAM1::END; i++) {
        busAccessors[i] = {&extBus, &ExtBus::read, &ExtBus::write};
    }

    /* 0xD000 - 0xDFFF */
    for (uint16_t i = Specs::MemoryLayout::WRAM2::START; i <= Specs::MemoryLayout::WRAM2::END; i++) {
        busAccessors[i] = {&extBus, &ExtBus::read, &ExtBus::write};
    }

    /* 0xE000 - 0xFDFF */
    for (uint16_t i = Specs::MemoryLayout::ECHO_RAM::START; i <= Specs::MemoryLayout::ECHO_RAM::END; i++) {
        busAccessors[i] = {&extBus, &ExtBus::read, &ExtBus::write};
    }

    /* 0xFE00 - 0xFE9F */
    for (uint16_t i = Specs::MemoryLayout::OAM::START; i <= Specs::MemoryLayout::OAM::END; i++) {
        busAccessors[i] = {&oamBus, &OamBus::read, &OamBus::write};
    }

    /* 0xFEA0 - 0xFEFF */
    for (uint16_t i = Specs::MemoryLayout::NOT_USABLE::START; i <= Specs::MemoryLayout::NOT_USABLE::END; i++) {
        busAccessors[i] = {&cpuBus, &CpuBus::read, &CpuBus::write};
    }

    /* 0xFF00 - 0xFFFF */
    for (uint16_t i = Specs::MemoryLayout::IO::START; i <= Specs::MemoryLayout::IO::END; i++) {
        busAccessors[i] = {&cpuBus, &CpuBus::read, &CpuBus::write};
    }

    /* 0xFF80 - 0xFFFE */
    for (uint16_t i = Specs::MemoryLayout::HRAM::START; i <= Specs::MemoryLayout::HRAM::END; i++) {
        busAccessors[i] = {&cpuBus, &CpuBus::read, &CpuBus::write};
    }

    /* 0xFFFF */
    busAccessors[Specs::MemoryLayout::IE] = {&cpuBus, &CpuBus::read, &CpuBus::write};
}

template <MmuDevice::Type d>
MmuSocket<d> MmuIO::socket() {
    return MmuSocket<d>(*this, lanes[d].data);
}

inline void MmuIO::saveState(Parcel& parcel) const {
    const auto saveLane = [&parcel](const BusLane& lane) {
        parcel.writeUInt8((uint8_t)lane.state);
        parcel.writeUInt16(lane.address);
        parcel.writeUInt8(*lane.data);
    };

    for (uint32_t i = 0; i < array_size(lanes); i++)
        saveLane(lanes[i]);
}

inline void MmuIO::loadState(Parcel& parcel) {
    const auto loadLane = [&parcel](BusLane& lane) {
        lane.state = (BusLane::State)parcel.readUInt8();
        lane.address = parcel.readUInt16();
        *lane.data = parcel.readUInt8();
    };

    for (uint32_t i = 0; i < array_size(lanes); i++)
        loadLane(lanes[i]);
}

template <>
inline void MmuIO::requestRead<MmuIO::Device::Cpu>(uint16_t address) {
    BusLane& cpuLane = lanes[Device::Cpu];
    const BusLane& dmaLane = lanes[Device::Dma];

    check(cpuLane.state == BusLane::State::None);
    cpuLane.state = BusLane::State::Read;

    if (dmaLane.state == BusLane::State::Read && busAt(address) == busAt(dmaLane.address)) {
        cpuLane.address = dmaLane.address;
    } else {
        cpuLane.address = address;
    }
}

template <>
inline void MmuIO::requestRead<MmuIO::Device::Dma>(uint16_t address) {
    BusLane& dmaLane = lanes[Device::Dma];
    check(dmaLane.state == BusLane::State::None);
    dmaLane.state = BusLane::State::Read;
    dmaLane.address = address;
}

template <>
inline void MmuIO::requestWrite<MmuIO::Device::Cpu>(uint16_t address) {
    const BusLane& dmaLane = lanes[Device::Dma];

    if (dmaLane.state == BusLane::State::Read && busAt(address) == busAt(dmaLane.address))
        return;

    BusLane& cpuLane = lanes[Device::Cpu];
    check(cpuLane.state == BusLane::State::None);
    cpuLane.state = BusLane::State::Write;
    cpuLane.address = address;
}

template <>
inline void MmuIO::requestWrite<MmuIO::Device::Dma>(uint16_t address) {
    BusLane& dmaLane = lanes[Device::Dma];
    check(dmaLane.state == BusLane::State::None);
    dmaLane.state = BusLane::State::Write;
    dmaLane.address = address;
}

inline IBus* MmuIO::busAt(uint16_t address) const {
    return busAccessors[address].bus;
}

inline Mmu::Mmu(IF_BOOTROM(BootRom& bootRom COMMA) ExtBus& extBus, CpuBus& cpuBus, VramBus& vramBus, OamBus& oamBus) :
    MmuIO(IF_BOOTROM(bootRom COMMA) extBus, cpuBus, vramBus, oamBus) {
}

inline void Mmu::tick_t0() {
    handleWriteRequest(lanes[Device::Cpu]);
    handleReadRequest(lanes[Device::Dma]);
}

inline void Mmu::tick_t1() {
}

inline void Mmu::tick_t2() {
    handleReadRequest(lanes[Device::Cpu]);
}

inline void Mmu::tick_t3() {
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

inline void Mmu::handleReadRequest(Mmu::BusLane& lane) {
    if (lane.state == BusLane::State::Read) {
        const BusAccess& busAccess = busAccessors[lane.address];
        (*lane.data) = (*busAccess.read)(busAccess.bus, lane.address);
        lane.state = BusLane::State::None;
    }
}

inline void Mmu::handleWriteRequest(Mmu::BusLane& lane) {
    if (lane.state == BusLane::State::Write) {
        const BusAccess& busAccess = busAccessors[lane.address];
        (*busAccess.write)(busAccess.bus, lane.address, *lane.data);
        lane.state = BusLane::State::None;
    }
}