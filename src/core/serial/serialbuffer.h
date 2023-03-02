#ifndef SERIALBUFFER_H
#define SERIALBUFFER_H

#include "serial.h"

class SerialBuffer : public SerialEndpoint {
public:
    SerialBuffer();
    ~SerialBuffer() override;

    uint8_t serialRead() override;
    void serialWrite(uint8_t) override;

    [[nodiscard]] const std::vector<uint8_t> &getData() const;
    void clearData();

protected:
    std::vector<uint8_t> buffer;
};

#endif // SERIALBUFFER_H