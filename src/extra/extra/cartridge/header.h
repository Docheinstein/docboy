#ifndef CARTRIDGEHEADER_H
#define CARTRIDGEHEADER_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

class ICartridge;

struct CartridgeHeader {
    static CartridgeHeader parse(const ICartridge& cartridge);

    std::array<uint8_t, 4> entry_point;
    std::array<uint8_t, 16> title;
    std::array<uint8_t, 48> nintendo_logo;
    uint8_t cgb_flag;
    std::array<uint8_t, 2> new_licensee_code;
    uint8_t sgb_flag;
    uint8_t cartridge_type;
    uint8_t rom_size;
    uint8_t ram_size;
    uint8_t destination_code;
    uint8_t old_licensee_code;
    uint8_t rom_version_number;
    uint8_t header_checksum;
    std::array<uint8_t, 2> global_checksum;

    std::vector<uint8_t> data;

    bool is_nintendo_logo_valid() const;
    bool is_header_checksum_valid() const;

    std::string title_as_string() const;
    std::string cgb_flag_description() const;
    std::string new_licensee_code_description() const;
    std::string cartridge_type_description() const;
    std::string rom_size_description() const;
    std::string ram_size_description() const;
    std::string old_licensee_code_description() const;
};

#endif // CARTRIDGEHEADER_H