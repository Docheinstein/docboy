#ifndef CORE_H
#define CORE_H

#include <memory>
#include "serial/link.h"
#include "joypad/joypad.h"

class IGameBoy;
class ISerialEndpoint;


class Core {
public:
    explicit Core(IGameBoy &gameboy);
    ~Core() = default;

    void loadROM(const std::string &rom);

    virtual void tick();
    virtual void frame();
    virtual bool isOn();

    virtual void setKey(IJoypad::Key, IJoypad::KeyState);

    void attachSerialLink(SerialLink::Plug &plug);
    void detachSerialLink();

protected:
    IGameBoy &gameboy;

    std::shared_ptr<SerialLink> serialLink;
};

#endif // CORE_H