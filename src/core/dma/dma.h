#ifndef DMA_H
#define DMA_H

#include "core/clock/clockable.h"
#include <cstdint>

class IBus;

class IDMA {
public:
    virtual ~IDMA() = default;
    virtual void transfer(uint16_t source) = 0;
};

class DMA : public IDMA, public IClockable {
public:
    explicit DMA(IBus& bus);
    void transfer(uint16_t source) override;
    void tick() override;

protected:
    IBus& bus;

    struct {
        bool active;
        uint16_t source;
        uint16_t cursor;
    } transferState;
};

#endif // DMA_H