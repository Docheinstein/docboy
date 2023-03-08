#include "ppu.h"
#include <stdexcept>

DebuggablePPU::DebuggablePPU(ILCD &display, IMemory &vram, IMemory &oam, IIO &io)
    : PPU(display, vram, oam, io) {

}

IDebuggablePPU::PPUState DebuggablePPU::getPPUState() const {
    switch (state) {
    case PPU::State::OAMScan:
        return IDebuggablePPU::PPUState::OAMScan;
    case PPU::State::PixelTransfer:
        return IDebuggablePPU::PPUState::PixelTransfer;
    case PPU::State::HBlank:
        return IDebuggablePPU::PPUState::HBlank;
    case PPU::State::VBlank:
        return IDebuggablePPU::PPUState::VBlank;
    }
    throw std::runtime_error("Unknown IDebuggablePPU::PPUState");
}

IDebuggablePPU::FetcherState DebuggablePPU::getFetcherState() const {
    if (fetcher.dots < 2)
        return IDebuggablePPU::FetcherState::GetTile;
    else if (fetcher.dots < 4)
        return IDebuggablePPU::FetcherState::GetTileDataLow;
    else if (fetcher.dots < 6)
        return IDebuggablePPU::FetcherState::GetTileDataHigh;
    else
        return IDebuggablePPU::FetcherState::Push;
}

uint32_t DebuggablePPU::getFetcherDots() const {
    return fetcher.dots;
}

uint32_t DebuggablePPU::getDots() const {
    return dots;
}

uint64_t DebuggablePPU::getCycles() const {
    return tCycles;
}

FIFO DebuggablePPU::getBgFifo() const {
    return bgFifo;
}

FIFO DebuggablePPU::getObjFifo() const {
    return objFifo;
}

uint8_t DebuggablePPU::getFetcherX() const {
    return fetcher.x8;
}

uint8_t DebuggablePPU::getFetcherY() const {
    return fetcher.y;
}

uint16_t DebuggablePPU::getFetcherLastTileMapAddr() const {
    return fetcher.scratchpad.tilemapAddr;
}

uint16_t DebuggablePPU::getFetcherLastTileAddr() const {
    return fetcher.scratchpad.tileAddr;
}

uint16_t DebuggablePPU::getFetcherLastTileDataAddr() const {
    return fetcher.scratchpad.tileDataAddr;
}

