#include "extra/serial/endpoints/console.h"

#include <algorithm>
#include <iostream>

#include "utils/hexdump.h"

SerialConsole::SerialConsole(std::ostream& output, uint32_t bufsize) :
    output {output},
    bufsize {bufsize} {
}

void SerialConsole::serial_write_bit(bool bit) {
    if (serial_write_bit_(bit) && (buffer[buffer.size() - 1] == '\n' || (buffer.size() > bufsize))) {
        flush();
    }
}

void SerialConsole::flush() {
    while (!buffer.empty()) {
        std::vector<uint8_t> line;
        auto newline = std::find(buffer.begin(), buffer.end(), '\n');
        auto last = newline + (newline != buffer.end() ? 1 : 0);
        std::copy(buffer.begin(), last, std::back_inserter(line));
        std::string dump =
            Hexdump<uint8_t>().show_addresses(false).show_ascii(true).num_columns(16).hexdump(line.data(), line.size());
        output << dump << std::endl;
        buffer.erase(buffer.begin(), last);
    }
}