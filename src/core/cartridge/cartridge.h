#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include "core/memory/memory.h"
#include <cstdint>
#include <string>
#include <vector>

class ICartridge : public IMemory {
public:
    struct Header {
        std::vector<uint8_t> entry_point;
        std::string title;
        std::vector<uint8_t> nintendo_logo;
        uint8_t cgb_flag;
        std::vector<uint8_t> new_licensee_code;
        uint8_t sgb_flag;
        uint8_t cartridge_type;
        uint8_t rom_size;
        uint8_t ram_size;
        uint8_t destination_code;
        uint8_t old_licensee_code;
        uint8_t rom_version_number;
        uint8_t header_checksum;
        std::vector<uint8_t> global_checksum;

        std::vector<uint8_t> data;

        [[nodiscard]] bool isValid() const;
        [[nodiscard]] bool isNintendoLogoValid() const;
        [[nodiscard]] bool isHeaderChecksumValid() const;
    };
    ~ICartridge() override = default;
    [[nodiscard]] virtual Header header() const = 0;
};

class Cartridge : public ICartridge {
public:
    explicit Cartridge(const std::vector<uint8_t>& data);
    explicit Cartridge(std::vector<uint8_t>&& data);

    [[nodiscard]] Header header() const override;

protected:
    std::vector<uint8_t> rom;
};

#endif // CARTRIDGE_H