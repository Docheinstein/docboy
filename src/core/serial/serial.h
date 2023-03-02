#ifndef SERIAL_H
#define SERIAL_H

#include <cstdint>
#include <vector>
#include <cstddef>

class SerialEndpoint {
public:
    virtual ~SerialEndpoint();
    virtual uint8_t serialRead();
    virtual void serialWrite(uint8_t);
};


class SerialLink {
public:
    SerialLink();
    SerialLink(SerialEndpoint *master, SerialEndpoint *slave);
    ~SerialLink();

    void setMaster(SerialEndpoint *master);
    void setSlave(SerialEndpoint *slave);

    void tick();

private:
    SerialEndpoint *master;
    SerialEndpoint *slave;
};


#endif // SERIAL_H