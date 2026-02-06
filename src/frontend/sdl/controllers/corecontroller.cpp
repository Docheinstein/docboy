#include "controllers/corecontroller.h"

#include <iostream>
#include <string>

#include "utils/io.h"
#include "utils/os.h"

#ifdef ENABLE_DEBUGGER
#include "docboy/debugger/backend.h"
#include "docboy/debugger/frontend.h"
#endif

CoreController::CoreController(Core& core) :
    core {core} {
}

#ifdef ENABLE_BOOTROM
void CoreController::load_boot_rom(const std::string& boot_rom_path) {
    if (!file_exists(boot_rom_path)) {
        std::cerr << "ERROR: failed to load '" << boot_rom_path << "'" << std::endl;
        exit(1);
    }

    boot_rom = Path {boot_rom_path};

    core.load_boot_rom(boot_rom_path);
}

bool CoreController::is_boot_rom_loaded() const {
    return !boot_rom.is_empty();
}
#endif

void CoreController::load_rom(const std::string& rom_path) {
    if (is_rom_loaded()) {
        // Eventually save current RAM state if a ROM is already loaded
        write_save();
    }

    if (!file_exists(rom_path)) {
        std::cerr << "ERROR: failed to load '" << rom_path << "'" << std::endl;
        exit(1);
    }

    rom = Path {rom_path};

    // Actually load ROM
    core.load_rom(rom_path);

    // Eventually load new RAM state
    load_save();

#ifdef ENABLE_DEBUGGER
    if (is_debugger_attached()) {
        // Reattach the debugger
        detach_debugger();
        attach_debugger(debugger.callbacks.on_pulling_command, debugger.callbacks.on_command_pulled);
    }
#endif
}

bool CoreController::is_rom_loaded() const {
    return !rom.is_empty();
}

Path CoreController::get_rom() const {
    return rom;
}

bool CoreController::write_save() const {
    if (!core.can_save()) {
        return false;
    }

    std::vector<uint8_t> data(core.get_save_size());
    core.save(data.data());

    bool ok;
    write_file(get_save_path(), data.data(), data.size(), &ok);

    if (!ok) {
        std::cerr << "WARN: failed to write save to '" << get_save_path() << "'" << std::endl;
        return false;
    }

    return true;
}

bool CoreController::load_save() const {
    if (!core.can_save()) {
        return false;
    }

    bool ok;
    std::vector<uint8_t> data = read_file(get_save_path(), &ok);

    if (!ok) {
        return false;
    }

    core.load(data.data());
    return true;
}

bool CoreController::write_state() const {
    std::vector<uint8_t> data(core.get_state_size());
    core.save_state(data.data());

    bool ok;
    write_file(get_state_path(), data.data(), data.size(), &ok);

    if (!ok) {
        std::cerr << "WARN: failed to write state to '" << get_state_path() << "'" << std::endl;
        return false;
    }

    return true;
}

bool CoreController::load_state() const {
    bool ok;
    std::vector<uint8_t> data = read_file(get_state_path(), &ok);

    if (!ok) {
        std::cerr << "WARN: failed to load state from '" << get_state_path() << "'" << std::endl;
        return false;
    }

    if (data.size() != core.get_state_size()) {
        std::cerr << "WARN: failed to load state: wrong size (expected=" << core.get_state_size()
                  << ", actual=" << data.size() << ")." << std::endl;
        std::cerr << "Maybe you saved with a different emulator version?" << std::endl;
        return false;
    }

    core.load_state(data.data());

    return true;
}

#ifdef ENABLE_DEBUGGER
bool CoreController::is_debugger_attached() const {
    return debugger.backend != nullptr;
}

bool CoreController::attach_debugger(const std::function<void()>& on_pulling_command,
                                     const std::function<bool(const std::string&)>& on_command_pulled,
                                     bool proceed_execution) {
    if (is_debugger_attached()) {
        return false;
    }

    // Setup the debugger
    debugger.backend = std::make_unique<DebuggerBackend>(core);
    debugger.frontend = std::make_unique<DebuggerFrontend>(*debugger.backend);
    core.attach_debugger(*debugger.backend);
    debugger.backend->attach_frontend(*debugger.frontend);
    DebuggerMemoryWatcher::attach_backend(*debugger.backend);

    // Setup the callbacks
    debugger.callbacks.on_pulling_command = on_pulling_command;
    debugger.callbacks.on_command_pulled = on_command_pulled;
    debugger.frontend->set_on_pulling_command_callback(debugger.callbacks.on_pulling_command);
    debugger.frontend->set_on_command_pulled_callback(debugger.callbacks.on_command_pulled);

    // Eventually load symbols from .sym file(s), if available in the same folder of the ROM(s)
#ifdef ENABLE_BOOTROM
    if (is_boot_rom_loaded()) {
        load_symbols_from_path(Path {boot_rom}.with_extension("sym"));
    }
#endif

    if (is_rom_loaded()) {
        load_symbols_from_path(Path {rom}.with_extension("sym"));
    }

    // Eventually load symbols from explicit file.
    if (!symbols.is_empty()) {
        load_symbols_from_path(symbols);
    }

    if (proceed_execution) {
        debugger.backend->proceed();
    }

    return true;
}

bool CoreController::detach_debugger() {
    if (!is_debugger_attached()) {
        return false;
    }

    debugger.backend = nullptr;
    debugger.frontend = nullptr;
    DebuggerMemoryWatcher::detach_backend();
    core.detach_debugger();

    return true;
}

void CoreController::load_symbols(const std::string& symbols_path) {
    symbols = Path {symbols_path};
    if (is_debugger_attached()) {
        load_symbols_from_path(symbols);
    }
}
#endif

const PixelRgb565* CoreController::get_framebuffer() const {
    return core.gb.lcd.get_pixels();
}

void CoreController::set_appearance(const Appearance& appearance) {
    core.gb.lcd.set_appearance(appearance);
}

void CoreController::set_key_mapping(SDL_Keycode keycode, Joypad::Key joypad_key) {
    // Remove previously bound keycode.
    if (const auto prev_keycode = joypad_map.find(joypad_key); prev_keycode != joypad_map.end()) {
        keycode_map.erase(keycode_map.find(prev_keycode->second));
    }

    keycode_map[keycode] = joypad_key;
    joypad_map[joypad_key] = keycode;
}

std::string CoreController::get_save_path() const {
    ASSERT(is_rom_loaded());
    return Path {rom}.with_extension("sav").string();
}

std::string CoreController::get_state_path() const {
    ASSERT(is_rom_loaded());
    return Path {rom}.with_extension("state").string();
}

#ifdef ENABLE_DEBUGGER
void CoreController::load_symbols_from_path(const Path& symbols_path) const {
    if (const std::string syms = symbols_path.string(); file_exists(syms)) {
        debugger.backend->load_symbols(syms);
    }
}
#endif

#ifdef ENABLE_TWO_PLAYERS_MODE
void CoreController::attach_serial_link(ISerialEndpoint& endpoint) {
    core.attach_serial_link(endpoint);
}

void CoreController::detach_serial_link() {
    core.detach_serial_link();
}

bool CoreController::is_serial_link_attached() const {
    return core.gb.serial.is_attached();
}

ISerialEndpoint& CoreController::get_serial_endpoint() {
    return core.gb.serial;
}
#endif