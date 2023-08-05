#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include "core/memory/memory.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

class ICartridge : public IMemory {
public:
    struct Header {
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

        [[nodiscard]] bool isValid() const;
        [[nodiscard]] bool isNintendoLogoValid() const;
        [[nodiscard]] bool isHeaderChecksumValid() const;

        [[nodiscard]] std::string titleAsString() const;
        [[nodiscard]] std::string cgbFlagDescription() const;
        [[nodiscard]] std::string newLicenseeCodeDescription() const;
        [[nodiscard]] std::string cartridgeTypeDescription() const;
        [[nodiscard]] std::string romSizeDescription() const;
        [[nodiscard]] std::string ramSizeDescription() const;
        [[nodiscard]] std::string oldLicenseeCodeDescription() const;
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