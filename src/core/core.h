#ifndef CORE_H
#define CORE_H

#include "gameboy.h"
#include "serial/serial.h"

class Core : public SerialEndpoint {
public:
    Core();
    ~Core() override = default;

    bool loadROM(const std::string &rom);
    virtual void start();
    void stop();

    void attachSerialLink(std::shared_ptr<SerialLink> serialLink);
    void detachSerialLink();

    uint8_t serialRead() override;
    void serialWrite(uint8_t) override;

protected:
    GameBoy gameboy;

    uint32_t divCounter;
    uint32_t timaCounter;

    std::shared_ptr<SerialLink> serialLink;

    bool running;

    virtual void tick();
};

#endif // CORE_H