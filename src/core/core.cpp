#include "core.h"
#include "cartridge/cartridgefactory.h"
#include "gameboy.h"

Core::Core(IGameBoy& gameboy) :
    gameboy(gameboy),
    serialLink() {
}

void Core::loadROM(const std::string& rom) {
    loadROM(CartridgeFactory::makeCartridge(rom));
}

void Core::loadROM(std::unique_ptr<ICartridge> cartridge) {
    gameboy.getCartridgeSlot().attachCartridge(std::move(cartridge));
}

void Core::attachSerialLink(SerialLink::Plug& plug) {
    gameboy.getSerialPort().attachSerialLink(plug);
}

void Core::detachSerialLink() {
    gameboy.getSerialPort().detachSerialLink();
}

void Core::frame() {
    ILCDIO& lcd = gameboy.getLCDIO();

    if (lcd.readLY() >= 144) {
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
