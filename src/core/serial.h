#ifndef SERIAL_H
#define SERIAL_H

#include <cstdint>
#include <vector>
#include <cstddef>

class ISerialEndpoint {
public:
    virtual ~ISerialEndpoint();
    virtual uint8_t serialRead();
    virtual void serialWrite(uint8_t) = 0;
};

class SerialBufferEndpoint : public ISerialEndpoint {
public:
    SerialBufferEndpoint();
    ~SerialBufferEndpoint() override;

    void serialWrite(uint8_t) override;

    [[nodiscard]] const std::vector<uint8_t> &getData() const;
    void clearData();

protected:
    std::vector<uint8_t> buffer;
};

class SerialLink {
public:
    SerialLink();
    ~SerialLink();

    void setMaster(ISerialEndpoint *e1);
    void setSlave(ISerialEndpoint *e1);

    void tick();

private:
    ISerialEndpoint *master;
    ISerialEndpoint *slave;

};

#endif // SERIAL_H