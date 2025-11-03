#include "docboy/ppu/dmgmodepal.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <optional>
#include <variant>

#include "docboy/cartridge/header.h"
#include "docboy/common/specs.h"

#include "utils/arrays.h"
#include "utils/asserts.h"
#include "utils/bits.h"
#include "utils/formatters.h"

namespace {
constexpr uint8_t PALETTE_SIZE = 8;
constexpr uint8_t NUM_PALETTES = 8;

struct DmgModePalettes {
    uint8_t bgp0[PALETTE_SIZE] {};
    uint8_t objp0[PALETTE_SIZE] {};
    uint8_t objp1[PALETTE_SIZE] {};
};

struct DmgModeAmbiguousPalettes {
    uint16_t disambiguation_letter {UINT16_MAX};
    DmgModePalettes palettes {};
};

using DmgModePalettesDisambiguation = std::array<DmgModeAmbiguousPalettes, 3>;

struct DmgModePalettesByChecksum {
    bool ambiguous {};

    DmgModePalettesDisambiguation ambiguous_palettes;
    DmgModePalettes non_ambiguous_palettes;
};

constexpr uint8_t AMBIGUOUS_TITLE_CHECKSUMS_START_INDEX = 0x41;

constexpr uint8_t TITLE_CHECKSUMS[] {/* 0x00 */ 0x00, 0x88, 0x16, 0x36, 0xD1, 0xDB, 0xF2, 0x3C,
                                     /* 0x08 */ 0x8C, 0x92, 0x3D, 0x5C, 0x58, 0xC9, 0x3E, 0x70,
                                     /* 0x10 */ 0x1D, 0x59, 0x69, 0x19, 0x35, 0xA8, 0x14, 0xAA,
                                     /* 0x18 */ 0x75, 0x95, 0x99, 0x34, 0x6F, 0x15, 0xFF, 0x97,
                                     /* 0x20 */ 0x4B, 0x90, 0x17, 0x10, 0x39, 0xF7, 0xF6, 0xA2,
                                     /* 0x28 */ 0x49, 0x4E, 0x43, 0x68, 0xE0, 0x8B, 0xF0, 0xCE,
                                     /* 0x30 */ 0x0C, 0x29, 0xE8, 0xB7, 0x86, 0x9A, 0x52, 0x01,
                                     /* 0x38 */ 0x9D, 0x71, 0x9C, 0xBD, 0x5D, 0x6D, 0x67, 0x3F,
                                     /* 0x40 */ 0x6B,
                                     /* --- Ambiguous --- */
                                     /* 0x41 */ 0xB3, 0x46, 0x28, 0xA5, 0xC6, 0xD3, 0x27,
                                     /* 0x48 */ 0x61, 0x18, 0x66, 0x6A, 0xBF, 0x0D, 0xF4};

constexpr std::optional<uint8_t> TITLE_FOURTH_LETTER_DISAMBIGUATION[][3] {
    /* 0x41 */ {'B', 'U', 'R'},
    /* 0x42 */ {'E', 'R', std::nullopt},
    /* 0x43 */ {'F', 'A', std::nullopt},
    /* 0x44 */ {'A', 'R', std::nullopt},
    /* 0x45 */ {'A', ' ', std::nullopt},
    /* 0x46 */ {'R', 'I', std::nullopt},
    /* 0x47 */ {'B', 'N', std::nullopt},
    /* 0x48 */ {'E', 'A', std::nullopt},
    /* 0x49 */ {'K', 'I', std::nullopt},
    /* 0x4A */ {'E', 'L', std::nullopt},
    /* 0x4B */ {'K', 'I', std::nullopt},
    /* 0x4C */ {' ', 'C', std::nullopt},
    /* 0x4D */ {'R', 'E', std::nullopt},
    /* 0x4E */ {'-', ' ', std::nullopt},
};

constexpr uint8_t PALETTE_TRIPLET_IDS_AND_FLAGS[] {/* 0x00 */ 0x7C, 0x08, 0x12, 0xA3, 0xA2, 0x07, 0x87, 0x4B,
                                                   /* 0x08 */ 0x20, 0x12, 0x65, 0xA8, 0x16, 0xA9, 0x86, 0xB1,
                                                   /* 0x10 */ 0x68, 0xA0, 0x87, 0x66, 0x12, 0xA1, 0x30, 0x3C,
                                                   /* 0x18 */ 0x12, 0x85, 0x12, 0x64, 0x1B, 0x07, 0x06, 0x6F,
                                                   /* 0x20 */ 0x6E, 0x6E, 0xAE, 0xAF, 0x6F, 0xB2, 0xAF, 0xB2,
                                                   /* 0x28 */ 0xA8, 0xAB, 0x6F, 0xAF, 0x86, 0xAE, 0xA2, 0xA2,
                                                   /* 0x30 */ 0x12, 0xAF, 0x13, 0x12, 0xA1, 0x6E, 0xAF, 0xAF,
                                                   /* 0x38 */ 0xAD, 0x06, 0x4C, 0x6E, 0xAF, 0xAF, 0x12, 0x7C,
                                                   /* 0x40 */ 0xAC,
                                                   /* --- Ambiguous --- */
                                                   /* 0x41 */ 0xA8, 0x6A, 0x6E, 0x13, 0xA0, 0x2D, 0xA8,
                                                   /* 0x48 */ 0x2B, 0xAC, 0x64, 0xAC, 0x6D, 0x87, 0xBC, 0x60,
                                                   /* 0x50 */ 0xB4, 0x13, 0x72, 0x7C, 0xB5, 0xAE, 0xAE, 0x7C,
                                                   /* 0x58 */ 0x7C, 0x65, 0xA2, 0x6C, 0x64, 0x85};

constexpr uint8_t PALETTE_OFFSETS[] {
    /* 0x00 */ 0x80, 0xB0, 0x40,
    /* 0x01 */ 0x88, 0x20, 0x68,
    /* 0x02 */ 0xDE, 0x00, 0x70,
    /* 0x03 */ 0xDE, 0x20, 0x78,
    /* 0x04 */ 0x20, 0x20, 0x38,
    /* 0x05 */ 0x20, 0xB0, 0x90,
    /* 0x06 */ 0x20, 0xB0, 0xA0,
    /* 0x07 */ 0xE0, 0xB0, 0xC0,
    /* 0x08 */ 0x98, 0xB6, 0x48,
    /* 0x09 */ 0x80, 0xE0, 0x50,
    /* 0x0A */ 0x1E, 0x1E, 0x58,
    /* 0x0B */ 0x20, 0xB8, 0xE0,
    /* 0x0C */ 0x88, 0xB0, 0x10,
    /* 0x0D */ 0x20, 0x00, 0x10,
    /* 0x0E */ 0x20, 0xE0, 0x18,
    /* 0x0F */ 0xE0, 0x18, 0x00,
    /* 0x10 */ 0x18, 0xE0, 0x20,
    /* 0x11 */ 0xA8, 0xE0, 0x20,
    /* 0x12 */ 0x18, 0xE0, 0x00,
    /* 0x13 */ 0x20, 0x18, 0xD8,
    /* 0x14 */ 0xC8, 0x18, 0xE0,
    /* 0x15 */ 0x00, 0xE0, 0x40,
    /* 0x16 */ 0x28, 0x28, 0x28,
    /* 0x17 */ 0x18, 0xE0, 0x60,
    /* 0x18 */ 0x20, 0x18, 0xE0,
    /* 0x19 */ 0x00, 0x00, 0x08,
    /* 0x1A */ 0xE0, 0x18, 0x30,
    /* 0x1B */ 0xD0, 0xD0, 0xD0,
    /* 0x1C */ 0x20, 0xE0, 0xE8,
    /* 0x1D */ 0xFF, 0x7F, 0xBF,
};

constexpr uint8_t PALETTES[] {
    /* 0x00 */ 0xFF, 0x7F, 0xBF, 0x32, 0xD0, 0x00, 0x00, 0x00,
    /* 0x01 */ 0x9F, 0x63, 0x79, 0x42, 0xB0, 0x15, 0xCB, 0x04,
    /* 0x02 */ 0xFF, 0x7F, 0x31, 0x6E, 0x4A, 0x45, 0x00, 0x00,
    /* 0x03 */ 0xFF, 0x7F, 0xEF, 0x1B, 0x00, 0x02, 0x00, 0x00,
    /* 0x04 */ 0xFF, 0x7F, 0x1F, 0x42, 0xF2, 0x1C, 0x00, 0x00,
    /* 0x05 */ 0xFF, 0x7F, 0x94, 0x52, 0x4A, 0x29, 0x00, 0x00,
    /* 0x06 */ 0xFF, 0x7F, 0xFF, 0x03, 0x2F, 0x01, 0x00, 0x00,
    /* 0x07 */ 0xFF, 0x7F, 0xEF, 0x03, 0xD6, 0x01, 0x00, 0x00,
    /* 0x08 */ 0xFF, 0x7F, 0xB5, 0x42, 0xC8, 0x3D, 0x00, 0x00,
    /* 0x09 */ 0x74, 0x7E, 0xFF, 0x03, 0x80, 0x01, 0x00, 0x00,
    /* 0x0A */ 0xFF, 0x67, 0xAC, 0x77, 0x13, 0x1A, 0x6B, 0x2D,
    /* 0x0B */ 0xD6, 0x7E, 0xFF, 0x4B, 0x75, 0x21, 0x00, 0x00,
    /* 0x0C */ 0xFF, 0x53, 0x5F, 0x4A, 0x52, 0x7E, 0x00, 0x00,
    /* 0x0D */ 0xFF, 0x4F, 0xD2, 0x7E, 0x4C, 0x3A, 0xE0, 0x1C,
    /* 0x0E */ 0xED, 0x03, 0xFF, 0x7F, 0x5F, 0x25, 0x00, 0x00,
    /* 0x0F */ 0x6A, 0x03, 0x1F, 0x02, 0xFF, 0x03, 0xFF, 0x7F,
    /* 0x10 */ 0xFF, 0x7F, 0xDF, 0x01, 0x12, 0x01, 0x00, 0x00,
    /* 0x11 */ 0x1F, 0x23, 0x5F, 0x03, 0xF2, 0x00, 0x09, 0x00,
    /* 0x12 */ 0xFF, 0x7F, 0xEA, 0x03, 0x1F, 0x01, 0x00, 0x00,
    /* 0x13 */ 0x9F, 0x29, 0x1A, 0x00, 0x0C, 0x00, 0x00, 0x00,
    /* 0x14 */ 0xFF, 0x7F, 0x7F, 0x02, 0x1F, 0x00, 0x00, 0x00,
    /* 0x15 */ 0xFF, 0x7F, 0xE0, 0x03, 0x06, 0x02, 0x20, 0x01,
    /* 0x16 */ 0xFF, 0x7F, 0xEB, 0x7E, 0x1F, 0x00, 0x00, 0x7C,
    /* 0x17 */ 0xFF, 0x7F, 0xFF, 0x3F, 0x00, 0x7E, 0x1F, 0x00,
    /* 0x18 */ 0xFF, 0x7F, 0xFF, 0x03, 0x1F, 0x00, 0x00, 0x00,
    /* 0x19 */ 0xFF, 0x03, 0x1F, 0x00, 0x0C, 0x00, 0x00, 0x00,
    /* 0x1A */ 0xFF, 0x7F, 0x3F, 0x03, 0x93, 0x01, 0x00, 0x00,
    /* 0x1B */ 0x00, 0x00, 0x00, 0x42, 0x7F, 0x03, 0xFF, 0x7F,
    /* 0x1C */ 0xFF, 0x7F, 0x8C, 0x7E, 0x00, 0x7C, 0x00, 0x00,
    /* 0x1D */ 0xFF, 0x7F, 0xEF, 0x1B, 0x80, 0x61, 0x00, 0x00,
};

constexpr void memcpy_uint8(uint8_t* dest, const uint8_t* src, size_t size) {
    // Unfortunately memcpy is not constexpr: explicitly copy with a for-loop
    for (size_t i = 0; i < size; i++) {
        dest[i] = src[i];
    }
}

constexpr DmgModePalettes compute_dmg_mode_non_ambiguous_palettes(uint8_t cksum_index) {
    // The following logic mimics the CGB bootrom

    DmgModePalettes palette {};

    // - Retrieve the palette index and the associated shuffling flags based on the title chekcsum index
    uint8_t palette_index = keep_bits<5>(PALETTE_TRIPLET_IDS_AND_FLAGS[cksum_index]);
    uint8_t shuffling_flags = get_bits_range<7, 5>(PALETTE_TRIPLET_IDS_AND_FLAGS[cksum_index]);

    // - Use the palette index to retrieve the offsets of the real palettes to use
    uint8_t palette_offset_0 = PALETTE_OFFSETS[3 * palette_index + 0];
    uint8_t palette_offset_1 = PALETTE_OFFSETS[3 * palette_index + 1];
    uint8_t palette_offset_2 = PALETTE_OFFSETS[3 * palette_index + 2];

    uint8_t palette_0[PALETTE_SIZE] {};
    uint8_t palette_1[PALETTE_SIZE] {};
    uint8_t palette_2[PALETTE_SIZE] {};

    // - Copy the palettes based on the offsets
    memcpy_uint8(palette_0, &PALETTES[palette_offset_0], PALETTE_SIZE);
    memcpy_uint8(palette_1, &PALETTES[palette_offset_1], PALETTE_SIZE);
    memcpy_uint8(palette_2, &PALETTES[palette_offset_2], PALETTE_SIZE);

    // - Use the shuffling flags to assign OBJP0 and OBJP1
    if (shuffling_flags == 0b000) {
        memcpy_uint8(palette.objp0, palette_2, PALETTE_SIZE);
        memcpy_uint8(palette.objp1, palette_2, PALETTE_SIZE);
    } else if (shuffling_flags == 0b001) {
        memcpy_uint8(palette.objp0, palette_0, PALETTE_SIZE);
        memcpy_uint8(palette.objp1, palette_2, PALETTE_SIZE);
    } else if (shuffling_flags == 0b010) {
        memcpy_uint8(palette.objp0, palette_2, PALETTE_SIZE);
        memcpy_uint8(palette.objp1, palette_0, PALETTE_SIZE);
    } else if (shuffling_flags == 0b011) {
        memcpy_uint8(palette.objp0, palette_0, PALETTE_SIZE);
        memcpy_uint8(palette.objp1, palette_0, PALETTE_SIZE);
    } else if (shuffling_flags == 0b0100) {
        memcpy_uint8(palette.objp0, palette_2, PALETTE_SIZE);
        memcpy_uint8(palette.objp1, palette_1, PALETTE_SIZE);
    } else if (shuffling_flags == 0b0101) {
        memcpy_uint8(palette.objp0, palette_0, PALETTE_SIZE);
        memcpy_uint8(palette.objp1, palette_1, PALETTE_SIZE);
    }

    // - BGP0 is always the last palette
    memcpy_uint8(palette.bgp0, palette_2, PALETTE_SIZE);

    return palette;
}

constexpr DmgModePalettes DEFAULT_PALETTES = compute_dmg_mode_non_ambiguous_palettes(0);

constexpr std::array<DmgModePalettesByChecksum, 256> compute_dmg_mode_palettes() {
    // Compute the DMG mode palettes associated with every possible checksum.
    // Some checksums might need further disambiguation at runtime based on the fourth letter.
    std::array<DmgModePalettesByChecksum, 256> palettes {};

    for (uint16_t i = 0; i < 256; i++) {
        auto cksum = static_cast<uint8_t>(i);

        // The following logic mimics the CGB bootrom

        // Seek the checksum in the title checksums table
        uint8_t cksum_index = 0;
        while (cksum_index < array_size(TITLE_CHECKSUMS) && TITLE_CHECKSUMS[cksum_index] != cksum) {
            cksum_index++;
        }

        if (cksum_index == array_size(TITLE_CHECKSUMS)) {
            // Checksum not found: use index 0 by default
            cksum_index = 0;
        }

        if (cksum_index < AMBIGUOUS_TITLE_CHECKSUMS_START_INDEX) {
            // Non-ambiguous checksum: compute palette
            palettes[cksum].ambiguous = false;
            palettes[cksum].non_ambiguous_palettes = compute_dmg_mode_non_ambiguous_palettes(cksum_index);
        } else {
            // Ambiguous checksum: compute all the known palettes for that checksum
            // (need further disambiguation at runtime based on the fourth letter)
            DmgModePalettesDisambiguation palettes_disambiguation {};

            const auto& disambiguation_letters =
                TITLE_FOURTH_LETTER_DISAMBIGUATION[cksum_index - AMBIGUOUS_TITLE_CHECKSUMS_START_INDEX];

            for (uint8_t letter_index = 0; letter_index < 3; letter_index++) {
                if (std::optional<uint8_t> optional_letter = disambiguation_letters[letter_index]) {
                    uint8_t non_ambiguous_cksum_index =
                        cksum_index + letter_index * array_size(TITLE_FOURTH_LETTER_DISAMBIGUATION);
                    palettes_disambiguation[letter_index].disambiguation_letter = *optional_letter;
                    palettes_disambiguation[letter_index].palettes =
                        compute_dmg_mode_non_ambiguous_palettes(non_ambiguous_cksum_index);
                }
            }

            palettes[cksum].ambiguous = true;
            palettes[cksum].ambiguous_palettes = palettes_disambiguation;
        }
    }

    return palettes;
}

uint8_t compute_title_checksum(const CartridgeHeader& header) {
    // The following logic mimics the CGB bootrom

    // Sum-up all the title's letters
    uint8_t checksum = 0;

    // TODO: up to 15 or 14?
    for (uint8_t i = 0; i < 16; i++) {
        checksum += header.title[i];
    }
    return checksum;
}

DmgModePalettes compute_dmg_mode_palettes_from_header(const CartridgeHeader& header) {
    static constexpr std::array<DmgModePalettesByChecksum, 256> DMG_MODE_PALETTES_BY_CHECKSUM =
        compute_dmg_mode_palettes();

    uint8_t cksum = compute_title_checksum(header);
    const auto& palettes_of_checksum = DMG_MODE_PALETTES_BY_CHECKSUM[cksum];

    if (!palettes_of_checksum.ambiguous) {
        // Non ambiguous palette
        return palettes_of_checksum.non_ambiguous_palettes;
    }

    // Ambiguous palette: disambiguate based on title's fourth letter
    uint8_t fourth_letter = header.title[3];
    const auto& palettes_disambiguation = palettes_of_checksum.ambiguous_palettes;
    for (const auto& [letter, palette] : palettes_disambiguation) {
        if (letter == fourth_letter) {
            return palette;
        }
    }

    // Use default palette otherwise
    return DEFAULT_PALETTES;
}
} // namespace

void load_dmg_mode_palettes_from_header(const CartridgeHeader& header, uint8_t* bg_palettes, uint8_t* obj_palettes) {
    ASSERT(!test_bit<Specs::Bits::Cartridge::CgbFlag::CGB_GAME>(header.cgb_flag()));

    DmgModePalettes palettes = DEFAULT_PALETTES;

    // Compute the DMG mode palettes (BGP0, OBJP0, OBJP1).
    // Note that the DMG mode palette selection is run only if either:
    // 1) The old licensee code is 0x01 (Nintendo)
    // 2) The old licensee code is 0x33 (use new licensee) and new licensee code is "01" (Nintendo)
    // Otherwise, the default palette is used instead.
    if (header.old_licensee_code == Specs::Cartridge::Header::OldLicensee::NINTENDO ||
        (header.old_licensee_code == Specs::Cartridge::Header::OldLicensee::USE_NEW_LICENSEE &&
         memcmp(header.new_licensee_code, Specs::Cartridge::Header::NewLicensee::NINTENDO,
                sizeof(header.new_licensee_code)) == 0)) {
        palettes = compute_dmg_mode_palettes_from_header(header);
    }

    memcpy(bg_palettes, palettes.bgp0, PALETTE_SIZE);
    memcpy(obj_palettes, palettes.objp0, PALETTE_SIZE);
    memcpy(obj_palettes + PALETTE_SIZE, palettes.objp1, PALETTE_SIZE);

    // Other BG palettes are all initialized as white (0x7FFF)
    for (uint32_t i = PALETTE_SIZE; i < NUM_PALETTES * PALETTE_SIZE; i += 2) {
        bg_palettes[i] = 0xFF;
        bg_palettes[i + 1] = 0x7F;
    }

    // Other OBJ palettes are all initialized as black (0x0000)
    memset(obj_palettes + 2 * PALETTE_SIZE, 0, (NUM_PALETTES - 2) * PALETTE_SIZE);
}
