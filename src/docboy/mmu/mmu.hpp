#include "docboy/boot/boot.h"
#include "docboy/bus/bus.h"
#include "docboy/bus/cpubus.h"
#include "docboy/bus/extbus.h"
#include "docboy/bus/oambus.h"
#include "docboy/bus/vrambus.h"
#include "mmu.h"

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

template <>
inline void MmuIO::requestRead<MmuIO::Device::Cpu>(uint16_t address, uint8_t& dest) {
    BusRequest& cpuReq = requests[Device::Cpu];
    const BusRequest& dmaReq = requests[Device::Dma];

    check(cpuReq.state == BusRequest::State::None);
    cpuReq.state = BusRequest::State::Read;
    cpuReq.read.destination = &dest;

    if (dmaReq.state == BusRequest::State::Read && busAt(address) == busAt(dmaReq.address)) {
        cpuReq.address = dmaReq.address;
    } else {
        cpuReq.address = address;
    }
}

template <>
inline void MmuIO::requestRead<MmuIO::Device::Dma>(uint16_t address, uint8_t& dest) {
    BusRequest& req = requests[Device::Dma];
    check(req.state == BusRequest::State::None);
    req.state = BusRequest::State::Read;
    req.address = address;
    req.read.destination = &dest;
}

template <>
inline void MmuIO::requestWrite<MmuIO::Device::Cpu>(uint16_t address, uint8_t value) {
    const BusRequest& dmaReq = requests[Device::Dma];

    if (dmaReq.state == BusRequest::State::Read && busAt(address) == busAt(dmaReq.address))
        return;

    BusRequest& req = requests[Device::Cpu];
    check(req.state == BusRequest::State::None);
    req.state = BusRequest::State::Write;
    req.address = address;
    req.write.value = value;
}

template <>
inline void MmuIO::requestWrite<MmuIO::Device::Dma>(uint16_t address, uint8_t value) {
    BusRequest& req = requests[Device::Dma];
    check(req.state == BusRequest::State::None);
    req.state = BusRequest::State::Write;
    req.address = address;
    req.write.value = value;
}

inline bool MmuIO::hasRequests() const {
    return requests[Device::Cpu].state == BusRequest::State::None &&
           requests[Device::Dma].state == BusRequest::State::None;
}

inline IBus* MmuIO::busAt(uint16_t address) const {
    return busAccessors[address].bus;
}

inline Mmu::Mmu(IF_BOOTROM(BootRom& bootRom COMMA) ExtBus& extBus, CpuBus& cpuBus, VramBus& vramBus, OamBus& oamBus) :
    MmuIO(IF_BOOTROM(bootRom COMMA) extBus, cpuBus, vramBus, oamBus) {
}

inline void Mmu::tick_t0() {
    handleWriteRequest(requests[Device::Cpu]);
    handleReadRequest(requests[Device::Dma]);
}

inline void Mmu::tick_t1() {
}

inline void Mmu::tick_t2() {
    handleReadRequest(requests[Device::Cpu]);
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

inline void Mmu::handleReadRequest(Mmu::BusRequest& request) {
    if (request.state == BusRequest::State::Read) {
        const BusAccess& busAccess = busAccessors[request.address];
        (*request.read.destination) = (*busAccess.read)(busAccess.bus, request.address);
        request.state = BusRequest::State::None;
    }
}

inline void Mmu::handleWriteRequest(Mmu::BusRequest& request) {
    if (request.state == BusRequest::State::Write) {
        const BusAccess& busAccess = busAccessors[request.address];
        (*busAccess.write)(busAccess.bus, request.address, request.write.value);
        request.state = BusRequest::State::None;
    }
}

template <MmuDevice::Type d>
MmuProxy<d>::MmuProxy(MmuIO& mmu) :
    mmu(mmu) {
}

template <MmuDevice::Type d>
void MmuProxy<d>::requestWrite(uint16_t address, uint8_t value) {
    mmu.requestWrite<d>(address, value);
}

template <MmuDevice::Type d>
void MmuProxy<d>::requestRead(uint16_t address, uint8_t& dest) {
    mmu.requestRead<d>(address, dest);
}