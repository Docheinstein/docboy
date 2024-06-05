template <Device::Type Dev>
MmuView<Dev>::MmuView(Mmu& mmu) :
    mmu(mmu) {
}

template <Device::Type Dev>
void MmuView<Dev>::read_request(uint16_t address) {
    mmu.read_request<Dev>(address);
}

template <Device::Type Dev>
uint8_t MmuView<Dev>::flush_read_request() {
    return mmu.flush_read_request<Dev>();
}

template <Device::Type Dev>
void MmuView<Dev>::write_request(uint16_t address) {
    return mmu.write_request<Dev>(address);
}

template <Device::Type Dev>
void MmuView<Dev>::flush_write_request(uint16_t value) {
    return mmu.flush_write_request<Dev>(value);
}

template <Device::Type Dev>
void Mmu::read_request(uint16_t address) {
    BusAccess& bus_access {bus_accessors[Dev][address]};
    requests[Dev] = &bus_access;
    bus_access.read_request(address);
}

template <Device::Type Dev>
uint8_t Mmu::flush_read_request() {
    return requests[Dev]->flush_read_request();
}

template <Device::Type Dev>
void Mmu::write_request(uint16_t address) {
    BusAccess& bus_access {bus_accessors[Dev][address]};
    requests[Dev] = &bus_access;
    bus_access.write_request(address);
}

template <Device::Type Dev>
void Mmu::flush_write_request(uint16_t value) {
    requests[Dev]->flush_write_request(value);
}

template <typename Bus, Device::Type Dev>
void Mmu::BusAccess::read_request(IBus* bus, uint16_t address) {
    static_cast<Bus*>(bus)->template read_request<Dev>(address);
}

template <typename Bus, Device::Type Dev>
uint8_t Mmu::BusAccess::flush_read_request(IBus* bus) {
    return static_cast<Bus*>(bus)->template flush_read_request<Dev>();
}

template <typename Bus, Device::Type Dev>
void Mmu::BusAccess::write_request(IBus* bus, uint16_t address) {
    static_cast<Bus*>(bus)->template write_request<Dev>(address);
}

template <typename Bus, Device::Type Dev>
void Mmu::BusAccess::flush_write_request(IBus* bus, uint8_t value) {
    static_cast<Bus*>(bus)->template flush_write_request<Dev>(value);
}

#ifdef ENABLE_DEBUGGER
template <typename Bus>
uint8_t Mmu::BusAccess::read_bus(const IBus* bus, uint16_t address) {
    return static_cast<const Bus*>(bus)->read_bus(address);
}
#endif

inline void Mmu::BusAccess::read_request(uint16_t address) {
    vtable.read_request(bus, address);
}

inline uint8_t Mmu::BusAccess::flush_read_request() {
    return vtable.flush_read_request(bus);
}

inline void Mmu::BusAccess::write_request(uint16_t address) {
    vtable.write_request(bus, address);
}

inline void Mmu::BusAccess::flush_write_request(uint8_t value) {
    vtable.flush_write_request(bus, value);
}

#ifdef ENABLE_DEBUGGER
inline uint8_t Mmu::BusAccess::read_bus(uint16_t address) const {
    return vtable.read_bus(bus, address);
}
#endif
