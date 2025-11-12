#include "docboy/memory/wram2.h"
#include "docboy/memory/randomdata.h"

void Wram2::reset() {
    Memory::reset(RANDOM_DATA);
}
