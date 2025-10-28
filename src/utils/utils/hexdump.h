#ifndef UTILSHEXDUMP_H
#define UTILSHEXDUMP_H

#include <cstdint>
#include <string>
#include <vector>

#include "utils/algo.h"
#include "utils/formatters.h"

template <typename T = uint8_t>
std::string hexdump(const T* data, uint32_t length, uint32_t base_addr = 0, bool show_addresses = true,
                    bool show_ascii = false, uint32_t columns = 32) {
    std::stringstream ss;
    std::string asciistr;

    size_t i;
    uint32_t line_cursor = 0;
    uint32_t until = divide_and_round_up(length, columns) * columns;

    for (i = 0; i < until; i++) {
        if (show_addresses && i % columns == 0) {
            ss << hex<uint32_t>(base_addr + i) << " | ";
        }

        T c;
        if (i < length) {
            c = data[i];
            hex(c, ss);
        } else {
            // Last row
            ss << "  ";
            c = ' ';
        }

        if (show_ascii) {
            asciistr += isprint(c) ? static_cast<char>(c) : '.';
        }
        if ((i + 1) % columns == 0) {
            if (show_ascii) {
                ss << " | " << asciistr;
                asciistr = "";
            }
            if ((i + 1) < length) {
                ss << std::endl;
            }
            line_cursor = 0;
        } else {
            ss << " ";
            if ((line_cursor + 1) % 8 == 0) {
                ss << " ";
            }
            ++line_cursor;
        }
    }

    return ss.str();
}

template <typename T = uint8_t>
class Hexdump {
public:
    Hexdump() = default;

    Hexdump& show_addresses(bool yes) {
        show_addresses_ = yes;
        return *this;
    }

    Hexdump& show_ascii(bool yes) {
        show_ascii_ = yes;
        return *this;
    }

    Hexdump& base_address(uint32_t addr) {
        base_address_ = addr;
        return *this;
    }

    Hexdump& num_columns(uint32_t cols) {
        num_columns_ = cols;
        return *this;
    }

    std::string hexdump(const T* data, uint32_t length) const {
        return ::hexdump(data, length, base_address_, show_addresses_, show_ascii_, num_columns_);
    }

private:
    uint32_t base_address_ {};
    bool show_addresses_ {};
    bool show_ascii_ {};
    uint8_t num_columns_ {32};
};

#endif // UTILSHEXDUMP_H