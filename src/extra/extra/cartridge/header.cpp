#include "extra/cartridge/header.h"

#include <cstring>
#include <map>

#include "docboy/cartridge/cartridge.h"
#include "docboy/common/specs.h"

#include "utils/bits.h"
#include "utils/formatters.h"
#include "utils/strings.h"

namespace {
const std::map<uint8_t, std::string> CGB_FLAG_DESCRIPTION_MAP = {
    {0x80, "The game supports CGB enhancements, but is backwards compatible with monochrome Game Boys"},
    {0xC0, "The game works on CGB only"},
};

const std::map<std::string, std::string> NEW_LICENSEE_CODE_DESCRIPTION_MAP = {
    {"00", "None"},
    {"01", "Nintendo R&D1"},
    {"08", "Capcom"},
    {"13", "Electronic Arts"},
    {"18", "Hudson Soft"},
    {"19", "b-ai"},
    {"20", "kss"},
    {"22", "pow"},
    {"24", "PCM Complete"},
    {"25", "san-x"},
    {"28", "Kemco Japan"},
    {"29", "seta"},
    {"30", "Viacom"},
    {"31", "Nintendo"},
    {"32", "Bandai"},
    {"33", "Ocean/Acclaim"},
    {"34", "Konami"},
    {"35", "Hector"},
    {"37", "Taito"},
    {"38", "Hudson"},
    {"39", "Banpresto"},
    {"41", "Ubi Soft"},
    {"42", "Atlus"},
    {"44", "Malibu"},
    {"46", "angel"},
    {"47", "Bullet-Proof"},
    {"49", "irem"},
    {"50", "Absolute"},
    {"51", "Acclaim"},
    {"52", "Activision"},
    {"53", "American sammy"},
    {"54", "Konami"},
    {"55", "Hi tech entertainment"},
    {"56", "LJN"},
    {"57", "Matchbox"},
    {"58", "Mattel"},
    {"59", "Milton Bradley"},
    {"60", "Titus"},
    {"61", "Virgin"},
    {"64", "LucasArts"},
    {"67", "Ocean"},
    {"69", "Electronic Arts"},
    {"70", "Infogrames"},
    {"71", "Interplay"},
    {"72", "Broderbund"},
    {"73", "sculptured"},
    {"75", "sci"},
    {"78", "THQ"},
    {"79", "Accolade"},
    {"80", "misawa"},
    {"83", "lozc"},
    {"86", "Tokuma Shoten Intermedia"},
    {"87", "Tsukuda Original"},
    {"91", "Chunsoft"},
    {"92", "Video system"},
    {"93", "Ocean/Acclaim"},
    {"95", "Varie"},
    {"96", "Yonezawa/s’pal"},
    {"97", "Kaneko"},
    {"99", "Pack in soft"},
    {"A4", "Konami (Yu-Gi-Oh!)"},
};

const std::map<uint8_t, std::string> CARTRIDGE_TYPE_DESCRIPTION_MAP = {
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

const std::map<uint8_t, std::string> OLD_LICENSEE_CODE_DESCRIPTION_MAP = {
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

const std::map<uint8_t, std::string> ROM_SIZE_DESCRIPTION_MAP = {
    {0x00, "32 KiB (2 banks)"},   {0x01, "64 KiB (4 banks)"},   {0x02, "128 KiB (8 banks)"},
    {0x03, "256 KiB (16 banks)"}, {0x04, "512 KiB (32 banks)"}, {0x05, "1 MiB (64 banks)"},
    {0x06, "2 MiB (128 banks)"},  {0x07, "4 MiB (256 banks)"},  {0x08, "8 MiB (512 banks)"},
    {0x52, "1.1 MiB (72 banks)"}, {0x53, "1.2 MiB (80 banks)"}, {0x54, "1.5 MiB (96 banks)"},
};

const std::map<uint8_t, std::string> RAM_SIZE_DESCRIPTION_MAP = {
    {0x00, "No RAM"},           {0x01, "2 KiB (unofficial)"}, {0x02, "8 KiB (1 banks)"},
    {0x03, "32 KiB (4 banks)"}, {0x04, "128 KiB (16 banks)"}, {0x05, "64 KiB (8 banks)"}};

std::string new_licensee_code_as_short_string(const CartridgeHeader& header) {
    std::string str;
    str.resize(2);
    for (uint8_t i = 0; i < sizeof(header.new_licensee_code); i++) {
        // We want to keep this display string of a fixed size of 2,
        // therefore any non-printable character (including \0) with a space.
        str[i] = isprint(header.new_licensee_code[i]) ? static_cast<char>(header.new_licensee_code[i]) : ' ';
    }
    return str;
}

} // namespace

namespace CartridgeHeaderHelpers {
std::string title_as_string(const CartridgeHeader& header) {
    const auto* title_cstr = reinterpret_cast<const char*>(header.title);
    std::string title {title_cstr, strnlen(title_cstr, sizeof(header.title))};
#ifdef ENABLE_CGB
    if (title.size() == 16) {
        // Bit 15 is used for CGB flag instead of title for CGB-era cartridges.
        uint8_t flag = cgb_flag(header);
        if (flag == Specs::Cartridge::Header::CgbFlag::DMG_AND_CGB ||
            flag == Specs::Cartridge::Header::CgbFlag::CGB_ONLY) {
            title[15] = '\0';
        }
    }
#endif

    return title;
}

std::string cgb_flag_description(const CartridgeHeader& header) {
    if (const auto it = CGB_FLAG_DESCRIPTION_MAP.find(cgb_flag(header)); it != CGB_FLAG_DESCRIPTION_MAP.end()) {
        return it->second;
    }
    return "Unknown";
}

std::string new_licensee_code_as_string(const CartridgeHeader& header) {
    return hex(header.new_licensee_code, 2) + " (\"" + new_licensee_code_as_short_string(header) + "\")";
}

std::string new_licensee_code_description(const CartridgeHeader& header) {
    if (const auto it = NEW_LICENSEE_CODE_DESCRIPTION_MAP.find(new_licensee_code_as_short_string(header));
        it != NEW_LICENSEE_CODE_DESCRIPTION_MAP.end()) {
        return it->second;
    }
    return "Unknown";
}

std::string cartridge_type_description(const CartridgeHeader& header) {
    if (const auto it = CARTRIDGE_TYPE_DESCRIPTION_MAP.find(header.cartridge_type);
        it != CARTRIDGE_TYPE_DESCRIPTION_MAP.end()) {
        return it->second;
    }
    return "Unknown";
}

std::string rom_size_description(const CartridgeHeader& header) {
    if (const auto it = ROM_SIZE_DESCRIPTION_MAP.find(header.rom_size); it != ROM_SIZE_DESCRIPTION_MAP.end()) {
        return it->second;
    }
    return "Unknown";
}

std::string ram_size_description(const CartridgeHeader& header) {
    if (const auto it = RAM_SIZE_DESCRIPTION_MAP.find(header.ram_size); it != RAM_SIZE_DESCRIPTION_MAP.end()) {
        return it->second;
    }
    return "Unknown";
}

std::string old_licensee_code_description(const CartridgeHeader& header) {
    if (const auto it = OLD_LICENSEE_CODE_DESCRIPTION_MAP.find(header.old_licensee_code);
        it != OLD_LICENSEE_CODE_DESCRIPTION_MAP.end()) {
        return it->second;
    }
    return "Unknown";
}
} // namespace CartridgeHeaderHelpers