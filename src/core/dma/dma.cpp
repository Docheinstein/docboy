#include "dma.h"
#include "core/bus/bus.h"

DMA::DMA(IBus& bus) :
    bus(bus),
    transferState() {
}

void DMA::loadState(IReadableState& state) {
    transferState.active = state.readBool();
    transferState.source = state.readUInt16();
    transferState.cursor = state.readUInt16();
}

void DMA::saveState(IWritableState& state) {
    state.writeBool(transferState.active);
    state.writeUInt16(transferState.source);
    state.writeUInt16(transferState.cursor);
}

void DMA::transfer(uint16_t source) {
    // TODO: what does happen if a transfer is already in progress?
    transferState = {.active = true, .source = source, .cursor = 0};
}

void DMA::tick() {
    if (!transferState.active)
        return;

    uint8_t data = bus.read(transferState.source + transferState.cursor);
    bus.write(MemoryMap::OAM::START + transferState.cursor, data);
    transferState.cursor++;

    if (transferState.cursor >= MemoryMap::OAM::SIZE)
        transferState.active = false;
}