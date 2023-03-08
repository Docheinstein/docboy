#ifndef DEBUGGERPPU_H
#define DEBUGGERPPU_H

#include "core/ppu/ppu.h"

class IDebuggablePPU : public virtual IPPU {
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

    ~IDebuggablePPU() override = default;

    [[nodiscard]] virtual PPUState getPPUState() const = 0;
    [[nodiscard]] virtual FIFO getBgFifo() const = 0;
    [[nodiscard]] virtual FIFO getObjFifo() const = 0;
    [[nodiscard]] virtual FetcherState getFetcherState() const = 0;
    [[nodiscard]] virtual uint32_t getFetcherDots() const = 0;
    [[nodiscard]] virtual uint8_t getFetcherX() const = 0;
    [[nodiscard]] virtual uint8_t getFetcherY() const = 0;
    [[nodiscard]] virtual uint16_t getFetcherLastTileMapAddr() const = 0;
    [[nodiscard]] virtual uint16_t getFetcherLastTileAddr() const = 0;
    [[nodiscard]] virtual uint16_t getFetcherLastTileDataAddr() const = 0;
    [[nodiscard]] virtual uint32_t getDots() const = 0;
    [[nodiscard]] virtual uint64_t getCycles() const = 0;
};

class DebuggablePPU : public IDebuggablePPU, public PPU {
public:
    explicit DebuggablePPU(ILCD &lcd, IMemory &vram, IMemory &oam, IIO &io);
    ~DebuggablePPU() override = default;

    [[nodiscard]] PPUState getPPUState() const override;
    [[nodiscard]] FIFO getBgFifo() const override;
    [[nodiscard]] FIFO getObjFifo() const override;
    [[nodiscard]] FetcherState getFetcherState() const override;
    [[nodiscard]] uint32_t getFetcherDots() const override;
    [[nodiscard]] uint8_t getFetcherX() const override;
    [[nodiscard]] uint8_t getFetcherY() const override;
    [[nodiscard]] uint16_t getFetcherLastTileMapAddr() const override;
    [[nodiscard]] uint16_t getFetcherLastTileAddr() const override;
    [[nodiscard]] uint16_t getFetcherLastTileDataAddr() const override;
    [[nodiscard]] uint32_t getDots() const override;
    [[nodiscard]] uint64_t getCycles() const override;
};

#endif // DEBUGGERPPU_H