#ifndef SERIALCONSOLE_H
#define SERIALCONSOLE_H

#include "serial.h"

class SerialConsoleEndpoint : public SerialBufferEndpoint {
public:
    explicit SerialConsoleEndpoint(int bufsize = -1 /* infinite */);
    ~SerialConsoleEndpoint() override;

    void serialWrite(uint8_t) override;
    void flush();

    void enable();
    void disable();

private:
    bool enabled;
    int bufsize;
};

#endif // SERIALCONSOLE_H