#include "ppu.h"
#include <stdexcept>
#include "core/io/lcd.h"

DebuggablePPU::DebuggablePPU(ILCD &lcd, ILCDIO &lcdIo, IInterruptsIO &interrupts, IMemory &vram, IMemory &oam) : PPU(
        lcd, lcdIo, interrupts, vram, oam) {

}

IPPUDebug::State DebuggablePPU::getState() {
    PPUState ppuState;
    switch (state) {
    case PPU::State::OAMScan:
        ppuState = PPUState::OAMScan;
        break;
    case PPU::State::PixelTransfer:
        ppuState = PPUState::PixelTransfer;
        break;
    case PPU::State::HBlank:
        ppuState = PPUState::HBlank;
        break;
    case PPU::State::VBlank:
        ppuState = PPUState::VBlank;
        break;
    default:
        throw std::runtime_error("Unknown PPU State");
    }

    IPPUDebug::FetcherState fetcherState;

    // TODO
    fetcherState = IPPUDebug::FetcherState::GetTile;
//    if (fetcher.dots < 2)
//        fetcherState = FetcherState::GetTile;
//    else if (fetcher.dots < 4)
//        fetcherState = FetcherState::GetTileDataLow;
//    else if (fetcher.dots < 6)
//        fetcherState = FetcherState::GetTileDataHigh;
//    else
//        fetcherState = FetcherState::Push;
//
//    uint8_t fetcherX = 8 * fetcher.x8;
//
    return {
        .ppu = {
            .state = ppuState,
            .dots = dots,
            .cycles = tCycles,
            .bgFifo = bgFifo,
            .objFifo = objFifo
        },
        .fetcher = {
            .state = fetcherState,
            .x = bgPrefetcher.x8,
            .y = lcdIo.readLY(),
            .lastTimeMapAddr = bgPrefetcher.scratchpad.tilemapAddr,
            .lastTileAddr = bgPrefetcher.scratchpad.tileAddr,
            .lastTileDataAddr = bgPrefetcher.out.tileDataAddr
        }
    };
}
