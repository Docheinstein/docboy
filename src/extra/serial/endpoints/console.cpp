#include "console.h"
#include "utils/hexdump.hpp"
#include <algorithm>
#include <iostream>

SerialConsole::SerialConsole(std::ostream& output, uint32_t bufsize) :
    output(output),
    bufsize(bufsize) {
}

void SerialConsole::serialWrite(uint8_t value) {
    SerialBuffer::serialWrite(value);
    if (value == '\n' || (buffer.size() > bufsize))
        flush();
}

void SerialConsole::flush() {
    while (!buffer.empty()) {
        std::vector<uint8_t> line;
        auto newline = std::find(buffer.begin(), buffer.end(), '\n');
        auto last = newline + (newline != buffer.end() ? 1 : 0);
        std::copy(buffer.begin(), last, std::back_inserter(line));
        std::string dump =
            Hexdump<uint8_t>().showAddresses(false).showAscii(true).setNumColumns(16).hexdump(line.data(), line.size());
        output << dump << std::endl;
        buffer.erase(buffer.begin(), last);
    }
}
