#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <iosfwd>

class Cartridge {
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

//    Cartridge() = default;
    explicit Cartridge(const std::vector<uint8_t> &data);
    explicit Cartridge(std::vector<uint8_t> &&data);
    virtual ~Cartridge() = default;

//    Cartridge & operator=(const Cartridge &other);
//    Cartridge & operator=(Cartridge &&other) noexcept;

    [[nodiscard]] Header header() const;

    [[nodiscard]] virtual uint8_t read(uint16_t address) const = 0;
    virtual void write(uint16_t address, uint8_t value) = 0;

protected:
    std::vector<uint8_t> rom;
};
#endif // CARTRIDGE_H