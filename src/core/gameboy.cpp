#include "gameboy.h"

GameBoy::GameBoy() : wram1(MemoryMap::WRAM1::SIZE), wram2(MemoryMap::WRAM2::SIZE), hram(MemoryMap::HRAM::SIZE), ie(1) {

}
