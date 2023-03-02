#include "gameboy.h"

GameBoy::GameBoy() :
        bus(wram1, wram2, io, hram, ie),
        cpu(bus) {

}
