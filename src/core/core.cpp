#include "core.h"
#include "core/gameboy.h"
#include "core/cartridge/cartridgefactory.h"

Core::Core(GameBoy &gameboy):
    gameboy(gameboy), serialLink() {
}

bool Core::loadROM(const std::string &rom) {
    auto c = CartridgeFactory::makeCartridge(rom);
    if (!c)
        return false;
    attachCartridge(std::move(c));
    return true;
}

void Core::attachCartridge(std::unique_ptr<Cartridge> cartridge) {
    gameboy.attachCartridge(std::move(cartridge));
}

void Core::attachSerialLink(SerialLink::Plug &plug) {
    gameboy.attachSerialLink(plug);
}

void Core::detachSerialLink() {
    gameboy.detachSerialLink();
}

void Core::frame() {
    uint8_t LY = gameboy.io.readLY();
    if (LY >= 144) {
        while (gameboy.io.readLY() != 0)
            tick();
    }
    while (gameboy.io.readLY() < 144)
        tick();
}

void Core::tick() {
    gameboy.clock.tick();
}


bool Core::isOn() {
    return true;
}

