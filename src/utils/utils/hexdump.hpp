#ifndef HEXDUMP_HPP
#define HEXDUMP_HPP

#include "formatters.hpp"
#include <cstdint>
#include <string>
#include <vector>

template <typename T = uint8_t>
std::string hexdump(const T* data, uint32_t length, uint32_t baseAddr = 0, bool showAddresses = true,
                    bool showAscii = false, uint32_t columns = 32) {
    std::stringstream ss;
    std::string asciistr;

    size_t i;
    for (i = 0; i < length; i++) {
        if (showAddresses && i % columns == 0) {
            ss << hex<uint32_t>(baseAddr + i) << " | ";
        }

        T c = data[i];
        hex(c, ss);
        if (showAscii) {
            //            if (c != '\n')
            asciistr += isprint(c) ? static_cast<char>(c) : '.';
        }
        if ((i + 1) % columns == 0) {
            if (showAscii) {
                ss << " | " << asciistr;
                asciistr = "";
            }
            if ((i + 1) < length) {
                ss << std::endl;
            }
        } else {
            ss << " ";
            if ((i + 1) % 8 == 0)
                ss << " ";
        }
    }
    if (showAscii) {
        i = i % columns;
        while (i < columns) {
            ss << " ";
            if (i + 1 < columns)
                ss << "  ";
            if ((i + 1) % 8 == 0)
                ss << " ";
            ++i;
        }
        ss << " | " << asciistr;
    }

    return ss.str();
}

template <typename T = uint8_t>
class Hexdump {
public:
    Hexdump() = default;

    Hexdump& showAddresses(bool yes) {
        addresses = yes;
        return *this;
    }

    Hexdump& showAscii(bool yes) {
        ascii = yes;
        return *this;
    }

    Hexdump& setBaseAddress(uint32_t addr) {
        baseAddress = addr;
        return *this;
    }

    Hexdump& setNumColumns(uint32_t cols) {
        columns = cols;
        return *this;
    }

    [[nodiscard]] std::string hexdump(const T* data, uint32_t length) const {
        return ::hexdump(data, length, baseAddress, addresses, ascii, columns);
    }

private:
    uint32_t baseAddress {};
    bool addresses {};
    bool ascii {};
    uint8_t columns {32};
};

#endif // HEXDUMP_HPP