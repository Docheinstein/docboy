#ifndef ENDPOINT_H
#define ENDPOINT_H

#include <cstdint>

class SerialEndpoint {
public:
    virtual ~SerialEndpoint() = default;
    virtual uint8_t serialRead() = 0;
    virtual void serialWrite(uint8_t) = 0;
};


#endif // ENDPOINT_H