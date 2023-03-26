#include "core.h"
#include "gameboy.h"
#include "cartridge/cartridgefactory.h"

Core::Core(IGameBoy &gameboy):
    gameboy(gameboy), serialLink() {
}

bool Core::loadROM(const std::string &rom) {
    auto c = CartridgeFactory::makeCartridge(rom);
    if (!c)
        return false;
    gameboy.getCartridgeSlot().attachCartridge(std::move(c));
    return true;
}

void Core::attachSerialLink(SerialLink::Plug &plug) {
    gameboy.getSerialPort().attachSerialLink(plug);
}

void Core::detachSerialLink() {
    gameboy.getSerialPort().detachSerialLink();
}

void Core::frame() {
    ILCDIO &lcd = gameboy.getLCDIO();

    uint8_t LY = lcd.readLY();
    if (LY >= 144) {
        while (isOn() && lcd.readLY() != 0)
            tick();
    }
    while (isOn() && lcd.readLY() < 144)
        tick();
}

void Core::tick() {
    gameboy.getClock().tick();
}

bool Core::isOn() {
    return true;
}

void Core::setKey(IJoypad::Key key, IJoypad::KeyState state) {
    gameboy.getJoypad().setKeyState(key, state);
}

