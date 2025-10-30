#ifndef DMGMODEPALETTES_H
#define DMGMODEPALETTES_H

#include <cstdint>

struct CartridgeHeader;

void load_dmg_mode_palettes_from_header(const CartridgeHeader& header, uint8_t* bg_palettes, uint8_t* obj_palettes);

#endif // DMGMODEPALETTES_H