#ifndef BUFFER_H
#define BUFFER_H

#include "core/serial/endpoint.h"
#include <vector>

class SerialBuffer : public ISerialEndpoint {
public:
    SerialBuffer();
    ~SerialBuffer() override;

    uint8_t serialRead() override;
    void serialWrite(uint8_t) override;

    [[nodiscard]] const std::vector<uint8_t>& getData() const;
    void clearData();

protected:
    std::vector<uint8_t> buffer;
};

#endif // BUFFER_H