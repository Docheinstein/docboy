#ifndef DEBUGGERPPU_H
#define DEBUGGERPPU_H

#include "core/ppu/ppu.h"

class IPPUDebug {
public:
    enum class PPUState {
        HBlank,
        VBlank,
        OAMScan,
        PixelTransfer
    };

    enum class FetcherState {
        Prefetcher,
        PixelSliceFetcher,
        Pushing
    };

    enum class FIFOType {
        Bg,
        Obj
    };

    struct State {
        struct {
            PPUState state;
            uint32_t dots;
            uint64_t cycles;
            PPU::PixelFIFO bgFifo;
            PPU::PixelFIFO objFifo;
            std::vector<PPU::OAMEntry> scanlineOamEntries;
        } ppu{};
        struct {
            FetcherState state;
            uint32_t dots;
            std::vector<PPU::OAMEntry> oamEntriesHit;
            struct {
                uint16_t address;
                std::vector<PPU::Pixel> data;
            } pixelSliceFetcherTile;
            FIFOType targetFifo;
        } fetcher{};
    };

    virtual ~IPPUDebug() = default;

    virtual State getState() = 0;
};

class DebuggablePPU : public IPPUDebug, public PPU {
public:
    explicit DebuggablePPU(
        ILCD &lcd,
        ILCDIO &lcdIo,
        IInterruptsIO &interrupts,
        IMemory &vram,
        IMemory &oam
    );
    ~DebuggablePPU() override = default;

    IPPUDebug::State getState() override;
};

#endif // DEBUGGERPPU_H