#include "hexdump.h"
#include "binutils.h"

std::string hexdump(const uint8_t* data, size_t length, size_t baseAddr, bool addresses, bool ascii, size_t columns) {
    std::stringstream ss;
    std::string asciistr;

    size_t i;
    for (i = 0; i < length; i++) {
        if (addresses && i % columns == 0) {
            ss << hex<uint32_t>(baseAddr + i) << " | ";
        }

        auto c = data[i];
        hex(c, ss);
        if (ascii) {
            //            if (c != '\n')
            asciistr += isprint(c) ? c : '.';
        }
        if ((i + 1) % columns == 0) {
            if (ascii) {
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
    if (ascii) {
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

Hexdump::Hexdump() :
    baseAddress(),
    addresses(),
    ascii(),
    columns(32) {
}

std::string Hexdump::hexdump(const uint8_t* data, size_t length) {
    return ::hexdump(data, length, baseAddress, addresses, ascii, columns);
}

std::string Hexdump::hexdump(const std::vector<uint8_t>& vector) {
    return hexdump(vector.data(), vector.size());
}

Hexdump& Hexdump::setBaseAddress(size_t addr) {
    baseAddress = addr;
    return *this;
}

Hexdump& Hexdump::showAddresses(bool yes) {
    addresses = yes;
    return *this;
}

Hexdump& Hexdump::showAscii(bool yes) {
    ascii = yes;
    return *this;
}

Hexdump& Hexdump::setNumColumns(size_t cols) {
    columns = cols;
    return *this;
}
