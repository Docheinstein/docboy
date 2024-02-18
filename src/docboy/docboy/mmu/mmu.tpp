template <Device::Type Dev>
MmuView<Dev>::MmuView(Mmu& mmu) :
    mmu(mmu) {
}

template <Device::Type Dev>
void MmuView<Dev>::readRequest(uint16_t address) {
    mmu.readRequest<Dev>(address);
}

template <Device::Type Dev>
uint8_t MmuView<Dev>::flushReadRequest() {
    return mmu.flushReadRequest<Dev>();
}

template <Device::Type Dev>
void MmuView<Dev>::writeRequest(uint16_t address) {
    return mmu.writeRequest<Dev>(address);
}

template <Device::Type Dev>
void MmuView<Dev>::flushWriteRequest(uint16_t value) {
    return mmu.flushWriteRequest<Dev>(value);
}

template <Device::Type Dev>
void Mmu::readRequest(uint16_t address) {
    BusAccess& busAccess {busAccessors[Dev][address]};
    requests[Dev] = &busAccess;
    busAccess.readRequest(address);
}

template <Device::Type Dev>
uint8_t Mmu::flushReadRequest() {
    return requests[Dev]->flushReadRequest();
}

template <Device::Type Dev>
void Mmu::writeRequest(uint16_t address) {
    BusAccess& busAccess {busAccessors[Dev][address]};
    requests[Dev] = &busAccess;
    busAccess.writeRequest(address);
}

template <Device::Type Dev>
void Mmu::flushWriteRequest(uint16_t value) {
    requests[Dev]->flushWriteRequest(value);
}

template <typename Bus, Device::Type Dev>
void Mmu::BusAccess::readRequest(IBus* bus, uint16_t address) {
    static_cast<Bus*>(bus)->template readRequest<Dev>(address);
}

template <typename Bus, Device::Type Dev>
uint8_t Mmu::BusAccess::flushReadRequest(IBus* bus) {
    return static_cast<Bus*>(bus)->template flushReadRequest<Dev>();
}

template <typename Bus, Device::Type Dev>
void Mmu::BusAccess::writeRequest(IBus* bus, uint16_t address) {
    static_cast<Bus*>(bus)->template writeRequest<Dev>(address);
}

template <typename Bus, Device::Type Dev>
void Mmu::BusAccess::flushWriteRequest(IBus* bus, uint8_t value) {
    static_cast<Bus*>(bus)->template flushWriteRequest<Dev>(value);
}

#ifdef ENABLE_DEBUGGER
template <typename Bus>
uint8_t Mmu::BusAccess::readBus(const IBus* bus, uint16_t address) {
    return static_cast<const Bus*>(bus)->readBus(address);
}
#endif

inline void Mmu::BusAccess::readRequest(uint16_t address) {
    vtable.readRequest(bus, address);
}

inline uint8_t Mmu::BusAccess::flushReadRequest() {
    return vtable.flushReadRequest(bus);
}

inline void Mmu::BusAccess::writeRequest(uint16_t address) {
    vtable.writeRequest(bus, address);
}

inline void Mmu::BusAccess::flushWriteRequest(uint8_t value) {
    vtable.flushWriteRequest(bus, value);
}

#ifdef ENABLE_DEBUGGER
inline uint8_t Mmu::BusAccess::readBus(uint16_t address) const {
    return vtable.readBus(bus, address);
}
#endif
