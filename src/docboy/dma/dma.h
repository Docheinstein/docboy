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

    void transfer(uint16_t address);
    void tick();

    void saveState(Parcel& parcel) const;
    void loadState(Parcel& parcel);

    [[nodiscard]] inline bool isTransferring() const {
        return state != TransferState::None;
    }

private:
    enum class TransferState : uint8_t {
        /* Not transferring */
        None,
        /* DMA transfer required: starting next cycle */
        Pending,
        /* DMA transfer running */
        Active,
        /* DMA Transfer just finished this cycle */
        Finished
    };

    Mmu& mmu;
    Oam& oam;

    TransferState state {TransferState::None};
    uint16_t source {};
    uint8_t cursor {};
};

#endif // DMA_H