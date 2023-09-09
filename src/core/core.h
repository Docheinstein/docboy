#ifndef CORE_H
#define CORE_H

#include "core/save/readable.h"
#include "core/save/writable.h"
#include "joypad/joypad.h"
#include "serial/link.h"
#include "state/processor.h"
#include <memory>

class ICartridge;
class IGameBoy;
class ISerialEndpoint;

class ICore {
public:
    ~ICore() = default;
    virtual void tick() = 0;
    virtual void frame() = 0;
    virtual bool isOn() = 0;
};

class Core : public ICore {
public:
    explicit Core(IGameBoy& gameboy);

    void loadSave(IReadableSave& save);
    void saveSave(IWritableSave& save);

    void loadState(IReadableState& state);
    void saveState(IWritableState& state);

    void loadROM(const std::string& rom);
    void loadROM(std::unique_ptr<ICartridge>);

    void tick() override;
    void frame() override;
    bool isOn() override;

    virtual void setKey(IJoypad::Key, IJoypad::KeyState);

    void attachSerialLink(SerialLink::Plug& plug);
    void detachSerialLink();

protected:
    IGameBoy& gameboy;

    std::shared_ptr<SerialLink> serialLink;
};

#endif // CORE_H