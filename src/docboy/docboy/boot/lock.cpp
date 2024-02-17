#include "lock.h"
#include "docboy/mmu/mmu.h"

BootLock::BootLock(Mmu& mmu) :
    mmu(mmu) {
}

void BootLock::lock() {
    mmu.lockBootRom();
}
