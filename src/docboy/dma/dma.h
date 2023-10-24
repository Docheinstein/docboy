#ifndef DMA_H
#define DMA_H

#include "docboy/debugger/macros.h"
#include "docboy/memory/fwd/oamfwd.h"
#include <cstdint>

class Mmu;

class Parcel;

class Dma {
public:
    DEBUGGABLE_CLASS();

    explicit Dma(Mmu& mmu, Oam& oam);

    void startTransfer(uint16_t address);
    void tick();

    void saveState(Parcel& parcel) const;
    void loadState(Parcel& parcel);

    [[nodiscard]] inline bool isTransferring() const {
        return transferring;
    }

private:
    Mmu& mmu;
    Oam& oam;

    struct {
        uint8_t state {};
        uint16_t source {};
    } request;

    bool transferring {};
    uint16_t source {};
    uint8_t cursor {};
};

#endif // DMA_H