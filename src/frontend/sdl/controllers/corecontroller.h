#ifndef CORECONTROLLER_H
#define CORECONTROLLER_H

#include <functional>
#include <map>

#include "docboy/core/core.h"

#include "utils/path.h"

#include "SDL3/SDL_keycode.h"

#ifdef ENABLE_DEBUGGER
class DebuggerBackend;
class DebuggerFrontend;
#endif

class CoreController {
public:
    explicit CoreController(Core& core);

    // Rom
    void load_rom(const std::string& rom_path);
    bool is_rom_loaded() const;
    Path get_rom() const;

    // Running/Pause
    void set_paused(bool paused_) {
        paused = paused_;
    }
    bool is_paused() const {
        return paused;
    }

    // Save
    bool write_save() const;
    bool load_save() const;

    // State
    bool write_state() const;
    bool load_state() const;

    // Debugger
#ifdef ENABLE_DEBUGGER
    bool is_debugger_attached() const;
    bool attach_debugger(const std::function<void()>& on_pulling_command = {},
                         const std::function<bool(const std::string&)>& on_command_pulled = {},
                         bool proceed_execution = false);
    bool detach_debugger();
#endif

    // Video
    const Lcd::PixelRgb565* get_framebuffer() const;

    // Emulation
    void frame() {
        core.frame();
    }

    // Input
    void send_key(SDL_Keycode key, Joypad::KeyState key_state) const {
        if (const auto it = keycode_map.find(key); it != keycode_map.end()) {
            core.set_key(it->second, key_state);
        }
    }

    void set_key_mapping(SDL_Keycode keycode, Joypad::Key joypad_key);

    const std::map<SDL_Keycode, Joypad::Key>& get_keycode_map() const {
        return keycode_map;
    }
    const std::map<Joypad::Key, SDL_Keycode>& get_joypad_map() const {
        return joypad_map;
    }

private:
    std::string get_save_path() const;
    std::string get_state_path() const;

    Core& core;

    struct {
        Path path {};
        bool is_loaded {};
    } rom;

    bool paused {true};

    std::map<SDL_Keycode, Joypad::Key> keycode_map;
    std::map<Joypad::Key, SDL_Keycode> joypad_map;

#ifdef ENABLE_DEBUGGER
    struct {
        std::unique_ptr<DebuggerBackend> backend {};
        std::unique_ptr<DebuggerFrontend> frontend {};

        struct {
            std::function<void()> on_pulling_command {};
            std::function<bool(const std::string&)> on_command_pulled {};
        } callbacks;
    } debugger;
#endif
};

#endif // CORECONTROLLER_H
