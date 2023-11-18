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
    enum class RequestState : uint8_t {
        /* DMA transfer has been requested this cycle */
        Requested = 2,
        /* DMA transfer is pending and will (re)start next cycle */
        Pending = 1,
        /* No DMA request */
        None = 0,
    };

    Mmu& mmu;
    Oam& oam;

    struct {
        RequestState state {};
        uint16_t source {};
    } request;

    bool transferring {};
    uint16_t source {};
    uint8_t cursor {};
};

#endif // DMA_H