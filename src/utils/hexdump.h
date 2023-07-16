#ifndef HEXDUMP_H
#define HEXDUMP_H

#include <cstdint>
#include <string>
#include <vector>

std::string hexdump(const uint8_t* data, size_t length, bool addr = true, bool ascii = false, size_t columns = 32);

class Hexdump {
public:
    Hexdump();
    Hexdump& showAddresses(bool yes);
    Hexdump& showAscii(bool yes);
    Hexdump& setBaseAddress(size_t addr);
    Hexdump& setNumColumns(size_t cols);

    std::string hexdump(const std::vector<uint8_t>& vec);
    std::string hexdump(const uint8_t* data, size_t length);

private:
    size_t baseAddress;
    bool addresses;
    bool ascii;
    size_t columns;
};

#endif // HEXDUMP_H