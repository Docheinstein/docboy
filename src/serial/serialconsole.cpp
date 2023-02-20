#include "serialconsole.h"
#include <iostream>
#include "utils/binutils.h"
#include <algorithm>

SerialConsoleEndpoint::SerialConsoleEndpoint(std::ostream &output, int bufsize)
    : output(output), bufsize(bufsize) {

}
SerialConsoleEndpoint::~SerialConsoleEndpoint() {

}

void SerialConsoleEndpoint::serialWrite(uint8_t value) {
    SerialBufferEndpoint::serialWrite(value);
    if (value == '\n' || (bufsize >= 0 && buffer.size() > bufsize))
        flush();
}

void SerialConsoleEndpoint::flush() {
    while (!buffer.empty()) {
        std::vector<uint8_t> line;
        auto newline = std::find(buffer.begin(), buffer.end(), '\n');
        auto last = newline + (newline != buffer.end() ? 1 : 0);
        std::copy(buffer.begin(), last, std::back_inserter(line));
        output << hexdump(line, false, true, 16) << std::endl;
        buffer.erase(buffer.begin(), last);
    }
}
