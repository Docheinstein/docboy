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
        GetTile,
        GetTileDataLow,
        GetTileDataHigh,
        Push
    };

    struct State {
        struct {
            PPUState state;
            uint32_t dots;
            uint64_t cycles;
            FIFO bgFifo;
            FIFO objFifo;
        } ppu;
        struct {
            FetcherState state;
            uint32_t dots;
            uint8_t x;
            uint8_t y;
            uint16_t lastTimeMapAddr;
            uint16_t lastTileAddr;
            uint16_t lastTileDataAddr;
        } fetcher;
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