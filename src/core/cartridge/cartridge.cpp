#include "cartridge.h"
#include <cstring>
#include <map>

static const std::map<uint8_t, std::string> CGB_FLAG_DESCRIPTION_MAP = {
    {0x80, "The game supports CGB enhancements, but is backwards compatible with monochrome Game Boys"},
    {0xC0, "The game works on CGB only"},
};

static const std::map<uint16_t, std::string> NEW_LICENSEE_CODE_DESCRIPTION_MAP = {
    {0x00, "None"},
    {0x01, "Nintendo R&D1"},
    {0x08, "Capcom"},
    {0x13, "Electronic Arts"},
    {0x18, "Hudson Soft"},
    {0x19, "b-ai"},
    {0x20, "kss"},
    {0x22, "pow"},
    {0x24, "PCM Complete"},
    {0x25, "san-x"},
    {0x28, "Kemco Japan"},
    {0x29, "seta"},
    {0x30, "Viacom"},
    {0x31, "Nintendo"},
    {0x32, "Bandai"},
    {0x33, "Ocean/Acclaim"},
    {0x34, "Konami"},
    {0x35, "Hector"},
    {0x37, "Taito"},
    {0x38, "Hudson"},
    {0x39, "Banpresto"},
    {0x41, "Ubi Soft"},
    {0x42, "Atlus"},
    {0x44, "Malibu"},
    {0x46, "angel"},
    {0x47, "Bullet-Proof"},
    {0x49, "irem"},
    {0x50, "Absolute"},
    {0x51, "Acclaim"},
    {0x52, "Activision"},
    {0x53, "American sammy"},
    {0x54, "Konami"},
    {0x55, "Hi tech entertainment"},
    {0x56, "LJN"},
    {0x57, "Matchbox"},
    {0x58, "Mattel"},
    {0x59, "Milton Bradley"},
    {0x60, "Titus"},
    {0x61, "Virgin"},
    {0x64, "LucasArts"},
    {0x67, "Ocean"},
    {0x69, "Electronic Arts"},
    {0x70, "Infogrames"},
    {0x71, "Interplay"},
    {0x72, "Broderbund"},
    {0x73, "sculptured"},
    {0x75, "sci"},
    {0x78, "THQ"},
    {0x79, "Accolade"},
    {0x80, "misawa"},
    {0x83, "lozc"},
    {0x86, "Tokuma Shoten Intermedia"},
    {0x87, "Tsukuda Original"},
    {0x91, "Chunsoft"},
    {0x92, "Video system"},
    {0x93, "Ocean/Acclaim"},
    {0x95, "Varie"},
    {0x96, "Yonezawa/s’pal"},
    {0x97, "Kaneko"},
    {0x99, "Pack in soft"},
    {0xA4, "Konami (Yu-Gi-Oh!)"},
};

static const std::map<uint8_t, std::string> CARTRIDGE_TYPE_DESCRIPTION_MAP = {
    {0x00, "ROM ONLY"},
    {0x01, "MBC1"},
    {0x02, "MBC1+RAM"},
    {0x03, "MBC1+RAM+BATTERY"},
    {0x05, "MBC2"},
    {0x06, "MBC2+BATTERY"},
    {0x08, "ROM+RAM"},
    {0x09, "ROM+RAM+BATTERY"},
    {0x0B, "MMM01"},
    {0x0C, "MMM01+RAM"},
    {0x0D, "MMM01+RAM+BATTERY"},
    {0x0F, "MBC3+TIMER+BATTERY"},
    {0x10, "MBC3+TIMER+RAM+BATTERY"},
    {0x11, "MBC3"},
    {0x12, "MBC3+RAM"},
    {0x13, "MBC3+RAM+BATTERY"},
    {0x19, "MBC5"},
    {0x1A, "MBC5+RAM"},
    {0x1B, "MBC5+RAM+BATTERY"},
    {0x1C, "MBC5+RUMBLE"},
    {0x1D, "MBC5+RUMBLE+RAM"},
    {0x1E, "MBC5+RUMBLE+RAM+BATTERY"},
    {0x20, "MBC6"},
    {0x22, "MBC7+SENSOR+RUMBLE+RAM+BATTERY"},
    {0xFC, "POCKET CAMERA"},
    {0xFD, "BANDAI TAMA5"},
    {0xFE, "HuC3"},
    {0xFF, "HuC1+RAM+BATTERY"},
};

static const std::map<uint8_t, std::string> OLD_LICENSEE_CODE_DESCRIPTION_MAP = {
    {0x00, "None"},
    {0x01, "Nintendo"},
    {0x08, "Capcom"},
    {0x09, "Hot-B"},
    {0x0A, "Jaleco"},
    {0x0B, "Coconuts Japan"},
    {0x0C, "Elite Systems"},
    {0x13, "EA (Electronic Arts)"},
    {0x18, "Hudsonsoft"},
    {0x19, "ITC Entertainment"},
    {0x1A, "Yanoman"},
    {0x1D, "Japan Clary"},
    {0x1F, "Virgin Interactive"},
    {0x24, "PCM Complete"},
    {0x25, "San-X"},
    {0x28, "Kotobuki Systems"},
    {0x29, "Seta"},
    {0x30, "Infogrames"},
    {0x31, "Nintendo"},
    {0x32, "Bandai"},
    {0x33, "Indicates that the New licensee code should be used instead."},
    {0x34, "Konami"},
    {0x35, "HectorSoft"},
    {0x38, "Capcom"},
    {0x39, "Banpresto"},
    {0x3C, ".Entertainment i"},
    {0x3E, "Gremlin"},
    {0x41, "Ubisoft"},
    {0x42, "Atlus"},
    {0x44, "Malibu"},
    {0x46, "Angel"},
    {0x47, "Spectrum Holoby"},
    {0x49, "Irem"},
    {0x4A, "Virgin Interactive"},
    {0x4D, "Malibu"},
    {0x4F, "U.S. Gold"},
    {0x50, "Absolute"},
    {0x51, "Acclaim"},
    {0x52, "Activision"},
    {0x53, "American Sammy"},
    {0x54, "GameTek"},
    {0x55, "Park Place"},
    {0x56, "LJN"},
    {0x57, "Matchbox"},
    {0x59, "Milton Bradley"},
    {0x5A, "Mindscape"},
    {0x5B, "Romstar"},
    {0x5C, "Naxat Soft"},
    {0x5D, "Tradewest"},
    {0x60, "Titus"},
    {0x61, "Virgin Interactive"},
    {0x67, "Ocean Interactive"},
    {0x69, "EA (Electronic Arts)"},
    {0x6E, "Elite Systems"},
    {0x6F, "Electro Brain"},
    {0x70, "Infogrames"},
    {0x71, "Interplay"},
    {0x72, "Broderbund"},
    {0x73, "Sculptered Soft"},
    {0x75, "The Sales Curve"},
    {0x78, "t.hq"},
    {0x79, "Accolade"},
    {0x7A, "Triffix Entertainment"},
    {0x7C, "Microprose"},
    {0x7F, "Kemco"},
    {0x80, "Misawa Entertainment"},
    {0x83, "Lozc"},
    {0x86, "Tokuma Shoten Intermedia"},
    {0x8B, "Bullet-Proof Software"},
    {0x8C, "Vic Tokai"},
    {0x8E, "Ape"},
    {0x8F, "I’Max"},
    {0x91, "Chunsoft Co."},
    {0x92, "Video System"},
    {0x93, "Tsubaraya Productions Co."},
    {0x95, "Varie Corporation"},
    {0x96, "Yonezawa/S’Pal"},
    {0x97, "Kaneko"},
    {0x99, "Arc"},
    {0x9A, "Nihon Bussan"},
    {0x9B, "Tecmo"},
    {0x9C, "Imagineer"},
    {0x9D, "Banpresto"},
    {0x9F, "Nova"},
    {0xA1, "Hori Electric"},
    {0xA2, "Bandai"},
    {0xA4, "Konami"},
    {0xA6, "Kawada"},
    {0xA7, "Takara"},
    {0xA9, "Technos Japan"},
    {0xAA, "Broderbund"},
    {0xAC, "Toei Animation"},
    {0xAD, "Toho"},
    {0xAF, "Namco"},
    {0xB0, "acclaim"},
    {0xB1, "ASCII or Nexsoft"},
    {0xB2, "Bandai"},
    {0xB4, "Square Enix"},
    {0xB6, "HAL Laboratory"},
    {0xB7, "SNK"},
    {0xB9, "Pony Canyon"},
    {0xBA, "Culture Brain"},
    {0xBB, "Sunsoft"},
    {0xBD, "Sony Imagesoft"},
    {0xBF, "Sammy"},
    {0xC0, "Taito"},
    {0xC2, "Kemco"},
    {0xC3, "Squaresoft"},
    {0xC4, "Tokuma Shoten Intermedia"},
    {0xC5, "Data East"},
    {0xC6, "Tonkinhouse"},
    {0xC8, "Koei"},
    {0xC9, "UFL"},
    {0xCA, "Ultra"},
    {0xCB, "Vap"},
    {0xCC, "Use Corporation"},
    {0xCD, "Meldac"},
    {0xCE, ".Pony Canyon or"},
    {0xCF, "Angel"},
    {0xD0, "Taito"},
    {0xD1, "Sofel"},
    {0xD2, "Quest"},
    {0xD3, "Sigma Enterprises"},
    {0xD4, "ASK Kodansha Co."},
    {0xD6, "Naxat Soft"},
    {0xD7, "Copya System"},
    {0xD9, "Banpresto"},
    {0xDA, "Tomy"},
    {0xDB, "LJN"},
    {0xDD, "NCS"},
    {0xDE, "Human"},
    {0xDF, "Altron"},
    {0xE0, "Jaleco"},
    {0xE1, "Towa Chiki"},
    {0xE2, "Yutaka"},
    {0xE3, "Varie"},
    {0xE5, "Epcoh"},
    {0xE7, "Athena"},
    {0xE8, "Asmik ACE Entertainment"},
    {0xE9, "Natsume"},
    {0xEA, "King Records"},
    {0xEB, "Atlus"},
    {0xEC, "Epic/Sony Records"},
    {0xEE, "IGS"},
    {0xF0, "A Wave"},
    {0xF3, "Extreme Entertainment"},
    {0xFF, "LJN"},
};

static const std::map<uint8_t, std::string> ROM_SIZE_DESCRIPTION_MAP = {
    {0x00, "32 KiB (2 banks)"},   {0x01, "64 KiB (4 banks)"},   {0x02, "128 KiB (8 banks)"},
    {0x03, "256 KiB (16 banks)"}, {0x04, "512 KiB (32 banks)"}, {0x05, "1 MiB (64 banks)"},
    {0x06, "2 MiB (128 banks)"},  {0x07, "4 MiB (256 banks)"},  {0x08, "8 MiB (512 banks)"},
    {0x52, "1.1 MiB (72 banks)"}, {0x53, "1.2 MiB (80 banks)"}, {0x54, "1.5 MiB (96 banks)"},
};

static const std::map<uint8_t, std::string> RAM_SIZE_DESCRIPTION_MAP = {{0x00, "No RAM"},
                                                                        {0x02, "8 KiB (1 banks)"},
                                                                        {0x03, "32 KiB (4 banks)"},
                                                                        {0x04, "128 KiB (16 banks)"},
                                                                        {0x05, "64 KiB (8 banks)"}};

bool Cartridge::Header::isValid() const {
    return isNintendoLogoValid() && isHeaderChecksumValid();
}

bool Cartridge::Header::isNintendoLogoValid() const {
    static const uint8_t NINTENDO_LOGO[] = {0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83,
                                            0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
                                            0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63,
                                            0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E};

    return memcmp(nintendo_logo.data(), NINTENDO_LOGO, sizeof(NINTENDO_LOGO)) == 0;
}

bool Cartridge::Header::isHeaderChecksumValid() const {
    uint8_t expected_header_checksum = 0;
    for (uint16_t address = 0x134 - 0x100; address <= 0x14C - 0x100; address++)
        expected_header_checksum = expected_header_checksum - data[address] - 1;

    return expected_header_checksum == header_checksum;
}

std::string ICartridge::Header::titleAsString() const {
    char titleString[title.size() + 1];
    memcpy(titleString, title.data(), title.size());
    titleString[title.size()] = '\0';
    return titleString;
}

std::string ICartridge::Header::cgbFlagDescription() const {
    if (const auto it = CGB_FLAG_DESCRIPTION_MAP.find(cgb_flag); it != CGB_FLAG_DESCRIPTION_MAP.end()) {
        return it->second;
    }
    return "Unknown";
}

std::string ICartridge::Header::newLicenseeCodeDescription() const {
    uint16_t new_licensee_code_ext = new_licensee_code[0] << 8 | new_licensee_code[1];
    if (const auto it = NEW_LICENSEE_CODE_DESCRIPTION_MAP.find(new_licensee_code_ext);
        it != NEW_LICENSEE_CODE_DESCRIPTION_MAP.end()) {
        return it->second;
    }
    return "Unknown";
}

std::string ICartridge::Header::cartridgeTypeDescription() const {
    if (const auto it = CARTRIDGE_TYPE_DESCRIPTION_MAP.find(cartridge_type);
        it != CARTRIDGE_TYPE_DESCRIPTION_MAP.end()) {
        return it->second;
    }
    return "Unknown";
}

std::string ICartridge::Header::romSizeDescription() const {
    if (const auto it = ROM_SIZE_DESCRIPTION_MAP.find(rom_size); it != ROM_SIZE_DESCRIPTION_MAP.end()) {
        return it->second;
    }
    return "Unknown";
}

std::string ICartridge::Header::ramSizeDescription() const {
    if (const auto it = RAM_SIZE_DESCRIPTION_MAP.find(ram_size); it != RAM_SIZE_DESCRIPTION_MAP.end()) {
        return it->second;
    }
    return "Unknown";
}

std::string ICartridge::Header::oldLicenseeCodeDescription() const {
    if (const auto it = OLD_LICENSEE_CODE_DESCRIPTION_MAP.find(old_licensee_code);
        it != OLD_LICENSEE_CODE_DESCRIPTION_MAP.end()) {
        return it->second;
    }
    return "Unknown";
}

Cartridge::Header Cartridge::header() const {
    auto memcpy_range = [](uint8_t* dest, const uint8_t* src, size_t from, size_t to) {
        memcpy(dest, src + from, to - from + 1);
    };

    auto memcpy_range_v = [&memcpy_range](std::vector<uint8_t>& dest, const uint8_t* src, size_t from, size_t to) {
        dest.resize(to - from + 1);
        memcpy_range(dest.data(), src, from, to);
    };

    Cartridge::Header h;

    auto raw_data = (uint8_t*)rom.data();

    // Raw data
    memcpy_range_v(h.data, raw_data, 0x100, 0x14F);

    // Entry point (0x100 - 0x103)
    memcpy_range(h.entry_point.data(), raw_data, 0x100, 0x103);

    // Nintendo Logo (0x104 - 0x133)
    memcpy_range(h.nintendo_logo.data(), raw_data, 0x104, 0x133);

    // Title (0x134 - 0x143)
    memcpy_range(h.title.data(), raw_data, 0x134, 0x143);

    // CGB flag (0x143)
    h.cgb_flag = raw_data[0x143];

    // New licensee code (0x144 - 0x145)
    memcpy_range(h.new_licensee_code.data(), raw_data, 0x144, 0x145);

    // SGB flag (0x146)
    h.sgb_flag = raw_data[0x146];

    // Cartridge type (0x147)
    h.cartridge_type = raw_data[0x147];

    // Cartridge type (0x148)
    h.rom_size = raw_data[0x148];

    // Cartridge type (0x149)
    h.ram_size = raw_data[0x149];

    // Destination code (0x14A)
    h.destination_code = raw_data[0x14A];

    // Old licensee code (0x14B)
    h.old_licensee_code = raw_data[0x14B];

    // ROM version number (0x14C)
    h.rom_version_number = raw_data[0x14C];

    // Header checksum (0x14D)
    h.header_checksum = raw_data[0x14D];

    // Global checksum (0x14E - 0x14F)
    memcpy_range(h.global_checksum.data(), raw_data, 0x14E, 0x14F);

    return h;
}

Cartridge::Cartridge(const std::vector<uint8_t>& data) :
    rom(data) {
}

Cartridge::Cartridge(std::vector<uint8_t>&& data) :
    rom(data) {
}
