#ifndef CORE_H
#define CORE_H

#include <memory>
#include "core/serial/link.h"

class GameBoy;
class Cartridge;
class SerialEndpoint;

class Core {
public:
    explicit Core(GameBoy &gameboy);
    ~Core() = default;

    bool loadROM(const std::string &rom);

    virtual void tick();
    virtual void frame();
    virtual bool isOn();

    void attachSerialLink(SerialLink::Plug &plug);
    void detachSerialLink();

protected:
    GameBoy &gameboy;

    std::shared_ptr<SerialLink> serialLink;

    virtual void attachCartridge(std::unique_ptr<Cartridge> cartridge);
};

#endif // CORE_H