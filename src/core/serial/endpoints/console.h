#ifndef CONSOLE_H
#define CONSOLE_H

#include "buffer.h"
#include <iosfwd>

class SerialConsole : public SerialBuffer {
public:
    explicit SerialConsole(std::ostream& output, size_t bufsize = SIZE_MAX);
    ~SerialConsole() override = default;

    void serialWrite(uint8_t) override;
    void flush();

private:
    std::ostream& output;
    size_t bufsize;
};

#endif // CONSOLE_H