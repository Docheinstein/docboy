#ifndef SERIALCONSOLE_H
#define SERIALCONSOLE_H

#include "serialbuffer.h"
#include <iosfwd>

class SerialConsole : public SerialBuffer {
public:
    explicit SerialConsole(std::ostream &output, size_t bufsize = SIZE_MAX);
    ~SerialConsole() override = default;

    void serialWrite(uint8_t) override;
    void flush();

private:
    std::ostream &output;
    size_t bufsize;
};

#endif // SERIALCONSOLE_H