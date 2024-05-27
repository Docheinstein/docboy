#include "corecontroller.h"
#include "utils/io.h"
#include "utils/os.h"
#include <iostream>
#include <string>

#ifdef ENABLE_DEBUGGER
#include "docboy/debugger/backend.h"
#include "docboy/debugger/frontend.h"
#endif

CoreController::CoreController(Core& core) :
    core(core) {
}

void CoreController::loadRom(const std::string& romPath) {
    if (rom.isLoaded) {
        // Eventually save current RAM state if a ROM is already loaded
        writeSave();
    }

    if (!file_exists(romPath)) {
        std::cerr << "WARN: failed to load '" << romPath << "'" << std::endl;
        exit(1);
    }

    rom.romPath = path(romPath);
    rom.isLoaded = true;

    // Actually load ROM
    core.loadRom(romPath);

    // Eventually load new RAM state
    loadSave();

#ifdef ENABLE_DEBUGGER
    if (isDebuggerAttached()) {
        // Reattach the debugger
        detachDebugger();
        attachDebugger(debugger.callbacks.onPullingCommand, debugger.callbacks.onCommandPulled, true);
    }
#endif
}

bool CoreController::isRomLoaded() const {
    return rom.isLoaded;
}

path CoreController::getRom() const {
    return rom.romPath;
}

bool CoreController::writeSave() const {
    if (!core.canSaveRam())
        return false;

    std::vector<uint8_t> data(core.getRamSaveSize());
    core.saveRam(data.data());

    bool ok;
    write_file(getSavePath(), data.data(), data.size(), &ok);

    return ok;
}

bool CoreController::loadSave() const {
    if (!core.canSaveRam())
        return false;

    bool ok;
    std::vector<uint8_t> data = read_file(getSavePath(), &ok);
    if (!ok)
        return false;

    core.loadRam(data.data());
    return true;
}

bool CoreController::writeState() const {
    std::vector<uint8_t> data(core.getStateSize());
    core.saveState(data.data());

    bool ok;
    write_file(getStatePath(), data.data(), data.size(), &ok);

    return ok;
}

bool CoreController::loadState() const {
    bool ok;
    std::vector<uint8_t> data = read_file(getStatePath(), &ok);
    if (!ok)
        return false;

    if (data.size() != core.getStateSize())
        return false;

    core.loadState(data.data());

    return ok;
}

#ifdef ENABLE_DEBUGGER
bool CoreController::isDebuggerAttached() const {
    return debugger.backend != nullptr;
}

bool CoreController::attachDebugger(const std::function<void()>& onPullingCommand,
                                    const std::function<bool(const std::string&)>& onCommandPulled,
                                    bool proceedExecution) {
    if (isDebuggerAttached())
        return false;

    // Setup the debugger
    debugger.backend = std::make_unique<DebuggerBackend>(core);
    debugger.frontend = std::make_unique<DebuggerFrontend>(*debugger.backend);
    core.attachDebugger(*debugger.backend);
    debugger.backend->attachFrontend(*debugger.frontend);
    DebuggerMemorySniffer::setObserver(&*debugger.backend);

    // Setup the callbacks
    debugger.callbacks.onPullingCommand = onPullingCommand;
    debugger.callbacks.onCommandPulled = onCommandPulled;
    debugger.frontend->setOnPullingCommandCallback(debugger.callbacks.onPullingCommand);
    debugger.frontend->setOnCommandPulledCallback(debugger.callbacks.onCommandPulled);

    if (proceedExecution)
        debugger.backend->proceed();

    return true;
}

bool CoreController::detachDebugger() {
    if (!isDebuggerAttached())
        return false;

    debugger.backend = nullptr;
    debugger.frontend = nullptr;
    DebuggerMemorySniffer::setObserver(nullptr);
    core.detachDebugger();

    return true;
}
#endif

const Lcd::PixelRgb565* CoreController::getFramebuffer() const {
    return core.gb.lcd.getPixels();
}

void CoreController::setKeyMapping(SDL_Keycode keycode, Joypad::Key joypadKey) {
    // Remove previously bound keycode.
    if (const auto prevKeycode = joypadMap.find(joypadKey); prevKeycode != joypadMap.end()) {
        keycodeMap.erase(keycodeMap.find(prevKeycode->second));
    }

    keycodeMap[keycode] = joypadKey;
    joypadMap[joypadKey] = keycode;
}

const std::map<SDL_Keycode, Joypad::Key>& CoreController::getKeycodeMap() const {
    return keycodeMap;
}

const std::map<Joypad::Key, SDL_Keycode>& CoreController::getJoypadMap() const {
    return joypadMap;
}

std::string CoreController::getSavePath() const {
    check(isRomLoaded());
    return rom.romPath.with_extension("sav").string();
}

std::string CoreController::getStatePath() const {
    check(isRomLoaded());
    return rom.romPath.with_extension("state").string();
}
