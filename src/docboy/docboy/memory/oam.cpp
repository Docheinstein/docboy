#include "docboy/memory/oam.h"

#ifndef ENABLE_CGB
#include "docboy/memory/randomdata.h"
#endif

void Oam::reset() {
#ifdef ENABLE_CGB
    Memory::reset();
#else
    Memory::reset(RANDOM_DATA);
#endif
}
