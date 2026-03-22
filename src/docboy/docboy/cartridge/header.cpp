#include "docboy/cartridge/header.h"

#include "docboy/common/specs.h"
#include "utils/bits.h"

#include <cstring>

namespace CartridgeHeaderHelpers {
uint8_t cgb_flag(const CartridgeHeader& header) {
    return header.title[15];
}

uint8_t title_checksum(const CartridgeHeader& header) {
    uint8_t cksum = 0;
    for (const uint8_t c : header.title) {
        cksum += c;
    }
    return cksum;
}

bool is_nintendo_game(const CartridgeHeader& header) {
    // A game is considered licensed by Nintendo either if:
    // 1) The old licensee code is 0x01 (Nintendo)
    // 2) The old licensee code is 0x33 (use new licensee) and the new licensee code is "01" (Nintendo).
    // Note: non-nintendo games in DMG mode follow a different execution flow
    // (e.g. no checksum is computed and the default palette is used instead).
    return header.old_licensee_code == Specs::Cartridge::Header::OldLicensee::NINTENDO ||
           (header.old_licensee_code == Specs::Cartridge::Header::OldLicensee::USE_NEW_LICENSEE &&
            memcmp(header.new_licensee_code, Specs::Cartridge::Header::NewLicensee::NINTENDO,
                   sizeof(header.new_licensee_code)) == 0);
}

bool is_cgb_game(const CartridgeHeader& header) {
    return test_bit<Specs::Bits::Cartridge::CgbFlag::CGB_GAME>(cgb_flag(header));
}

bool is_copy_logo_title_checksum(uint8_t cksum) {
    return cksum == 0x43 || cksum == 0x58;
}

bool is_dmg_mode_copy_logo(const CartridgeHeader& header) {
    // For DMG mode games, if checksum of title is 0x43 or 0x58, the execution follows a
    // different flow and part of nintendo logo is copied to a region of VRAM.
    return !is_cgb_game(header) && is_copy_logo_title_checksum(title_checksum(header));
}
} // namespace CartridgeHeaderHelpers