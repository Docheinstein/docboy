#ifndef BUFFER_H
#define BUFFER_H

#include "docboy/serial/endpoint.h"
#include <vector>

class SerialBuffer : public ISerialEndpoint {
public:
    uint8_t serialRead() override;
    void serialWrite(uint8_t) override;

    std::vector<uint8_t> buffer;
};

#endif // BUFFER_H