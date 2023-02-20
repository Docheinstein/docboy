#ifndef SERIALCONSOLE_H
#define SERIALCONSOLE_H

#include "serialbuffer.h"
#include <iosfwd>

class SerialConsoleEndpoint : public SerialBufferEndpoint {
public:
    explicit SerialConsoleEndpoint(std::ostream &output, int bufsize = -1 /* infinite */);
    ~SerialConsoleEndpoint() override;

    void serialWrite(uint8_t) override;
    void flush();

private:
    std::ostream &output;
    int bufsize;
};

#endif // SERIALCONSOLE_H