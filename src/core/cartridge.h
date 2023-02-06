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

    static std::optional<Cartridge> fromFile(const std::string &filename);

    Cartridge();
    ~Cartridge();

    Cartridge(const Cartridge &other);
    Cartridge(Cartridge &&other) noexcept;

    Cartridge & operator=(const Cartridge &other);
    Cartridge & operator=(Cartridge &&other) noexcept;

    friend std::ostream & operator<<(std::ostream &os, const Cartridge &cartridge);

    [[nodiscard]] Header header() const;

    uint8_t operator[](size_t index) const;

private:
    std::vector<uint8_t> data;
};
#endif // CARTRIDGE_H