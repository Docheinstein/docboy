#ifndef OAM_H
#define OAM_H

#include "docboy/common/specs.h"
#include "docboy/memory/memory.h"

class Oam : public Memory<Specs::MemoryLayout::OAM::START, Specs::MemoryLayout::OAM::END> {
public:
    void reset();
};

#endif // OAM_H