#ifndef DEBUGGERPPU_H
#define DEBUGGERPPU_H

#include "core/ppu/ppu.h"

class IPPUDebug {
public:
    enum class PPUState { HBlank, VBlank, OAMScan, PixelTransfer };

    enum class FetcherState { Prefetcher, PixelSliceFetcher, Pushing };

    enum class FIFOType { Bg, Obj };

    struct State {
        struct {
            bool isOn;
            PPUState state;
            uint32_t dots;
            uint64_t cycles;
            PPU::BGPixelFIFO bgFifo;
            PPU::OBJPixelFIFO objFifo;
            std::vector<PPU::OAMEntryFetchInfo> scanlineOamEntries;
        } ppu {};
        struct {
            FetcherState state;
            uint32_t dots;
            std::vector<PPU::OAMEntryFetchInfo> oamEntriesHit;
            struct {
                uint16_t address;
                std::vector<PPU::Pixel> data;
            } pixelSliceFetcherTile;
            FIFOType targetFifo;
        } fetcher {};
    };

    virtual ~IPPUDebug() = default;

    virtual State getState() = 0;

    virtual bool isNewFrame() = 0; // fast way to check whether the PPU is on the first dot of a new frame
};

class DebuggablePPU : public IPPUDebug, public PPU {
public:
    explicit DebuggablePPU(ILCD& lcd, ILCDIO& lcdIo, IInterruptsIO& interrupts, IMemory& vram, IMemory& oam);
    ~DebuggablePPU() override = default;

    IPPUDebug::State getState() override;

    bool isNewFrame() override;
};

#endif // DEBUGGERPPU_H