#ifndef CORE_H
#define CORE_H

#include "gameboy.h"
#include "serial/serial.h"

class Core : public SerialEndpoint {
public:
    Core(GameBoy &gameboy);
    ~Core() override = default;

    bool loadROM(const std::string &rom);

    virtual void start();
    virtual void tick(); // TODO: here?

    void stop();

    void attachSerialLink(std::shared_ptr<SerialLink> serialLink);
    void detachSerialLink();

    uint8_t serialRead() override;
    void serialWrite(uint8_t) override;

protected:
    GameBoy &gameboy;

    // TODO: move inside cpu
    uint32_t divCounter;
    uint32_t timaCounter;

    std::shared_ptr<SerialLink> serialLink;

    bool running;
};

#endif // CORE_H