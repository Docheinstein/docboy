#ifndef CONSOLE_H
#define CONSOLE_H

#include <iosfwd>

#include "extra/serial/endpoints/buffer.h"

class SerialConsole : public SerialBuffer {
public:
    explicit SerialConsole(std::ostream& output, uint32_t bufsize = UINT32_MAX);
    ~SerialConsole() override = default;

    void serial_write(uint8_t) override;
    void flush();

private:
    std::ostream& output;
    uint32_t bufsize;
};

#endif // CONSOLE_H