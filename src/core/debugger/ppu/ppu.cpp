#include "ppu.h"
#include "core/io/lcd.h"
#include "utils/binutils.h"
#include <stdexcept>

DebuggablePPU::DebuggablePPU(ILCD& lcd, ILCDIO& lcdIo, IInterruptsIO& interrupts, IMemory& vram, IMemory& oam) :
    PPU(lcd, lcdIo, interrupts, vram, oam) {
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
    fetcherState = IPPUDebug::FetcherState::Prefetcher;

    switch (fetcher.state) {
    case Fetcher::State::Prefetcher:
        fetcherState = IPPUDebug::FetcherState::Prefetcher;
        break;
    case Fetcher::State::PixelSliceFetcher:
        fetcherState = IPPUDebug::FetcherState::PixelSliceFetcher;
        break;
    case Fetcher::State::Pushing:
        fetcherState = IPPUDebug::FetcherState::Pushing;
        break;
    }

    std::vector<PPU::Pixel> pixelSliceFetcherTileData;

    if (fetcher.state == PPU::Fetcher::State::Pushing && fetcher.pixelSliceFetcher.isTileDataReady()) {
        uint8_t low = fetcher.pixelSliceFetcher.getTileDataLow();
        uint8_t high = fetcher.pixelSliceFetcher.getTileDataHigh();

        for (int b = 7; b >= 0; b--)
            pixelSliceFetcherTileData.emplace_back((get_bit(low, b) ? 0b01 : 0b00) | (get_bit(high, b) ? 0b10 : 0b00));
    }

    return {.ppu = {.state = ppuState,
                    .dots = dots,
                    .cycles = tCycles,
                    .bgFifo = bgFifo,
                    .objFifo = objFifo,
                    .scanlineOamEntries = scanlineOamEntries},
            .fetcher = {.state = fetcherState,
                        .dots = fetcher.dots,
                        .oamEntriesHit = fetcher.oamEntriesHit,
                        .pixelSliceFetcherTile = {.address = fetcher.pixelSliceFetcher.tileDataAddr,
                                                  .data = pixelSliceFetcherTileData},
                        .targetFifo = fetcher.targetFifo == PPU::Fetcher::FIFOType::Bg ? IPPUDebug::FIFOType::Bg
                                                                                       : IPPUDebug::FIFOType::Obj}};
}
