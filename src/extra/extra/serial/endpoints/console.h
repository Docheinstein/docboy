#ifndef SERIALCONSOLE_H
#define SERIALCONSOLE_H

#include <iosfwd>

#include "extra/serial/endpoints/buffer.h"

class SerialConsole : public SerialBuffer {
public:
    explicit SerialConsole(std::ostream& output, uint32_t bufsize = UINT32_MAX);
    ~SerialConsole() override = default;

    void serial_write_bit(bool bit) override;

    void flush();

private:
    std::ostream& output;
    uint32_t bufsize;
};

#endif // SERIALCONSOLE_H