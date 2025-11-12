#include "docboy/memory/wram1.h"
#include "docboy/memory/randomdata.h"

void Wram1::reset() {
    Memory::reset(RANDOM_DATA);
}
