#ifndef CORE_H
#define CORE_H

#include "gameboy.h"
#include "serial/serial.h"
#include <chrono>

class Core : public SerialEndpoint {
public:
    explicit Core(GameBoy &gameboy);
    ~Core() override = default;

    bool loadROM(const std::string &rom);

    virtual void start();
    virtual bool tick();

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
    uint8_t clk;

    std::chrono::high_resolution_clock::time_point lastTick;

    virtual void attachCartridge(std::unique_ptr<Cartridge> cartridge);
};

#endif // CORE_H