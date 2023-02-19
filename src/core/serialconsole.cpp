#include "serialconsole.h"
#include <iostream>
#include "utils/binutils.h"

SerialConsoleEndpoint::SerialConsoleEndpoint(int bufsize) : bufsize(bufsize), enabled(true) {

}

SerialConsoleEndpoint::~SerialConsoleEndpoint() {

}

void SerialConsoleEndpoint::serialWrite(uint8_t value) {
    if (!enabled)
        return;
    SerialBufferEndpoint::serialWrite(value);
    if (value == '\n' || (bufsize >= 0 && buffer.size() > bufsize))
        flush();
}

void SerialConsoleEndpoint::flush() {
    if (!enabled)
        return;
    if (!buffer.empty()) {
        std::cerr << hexdump(buffer, false, true, 16) << std::endl;
        buffer.clear();
    }
}

void SerialConsoleEndpoint::enable() {
    enabled = true;
}

void SerialConsoleEndpoint::disable() {
    enabled = false;
}

