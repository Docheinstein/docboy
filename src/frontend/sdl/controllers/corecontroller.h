#ifndef CORECONTROLLER_H
#define CORECONTROLLER_H

#include "SDL3/SDL_keycode.h"
#include "docboy/core/core.h"
#include "utils/path.h"
#include <functional>
#include <map>

#ifdef ENABLE_DEBUGGER
class DebuggerBackend;
class DebuggerFrontend;
#endif

class CoreController {
public:
    explicit CoreController(Core& core);

    // Rom
    void loadRom(const std::string& romPath);
    bool isRomLoaded() const;
    path getRom() const;

    // Running/Pause
    void setPaused(bool paused_) {
        paused = paused_;
    }
    bool isPaused() const {
        return paused;
    }

    // Save
    bool writeSave() const;
    bool loadSave() const;

    // State
    bool writeState() const;
    bool loadState() const;

    // Debugger
#ifdef ENABLE_DEBUGGER
    bool isDebuggerAttached() const;
    bool attachDebugger(const std::function<void()>& onPullingCommand = {},
                        const std::function<bool(const std::string&)>& onCommandPulled = {},
                        bool proceedExecution = false);
    bool detachDebugger();
#endif

    // Video
    const Lcd::PixelRgb565* getFramebuffer() const;

    // Emulation
    void frame() {
        core.frame();
    }

    // Input
    void sendKey(SDL_Keycode key, Joypad::KeyState keyState) const {
        if (const auto it = keycodeMap.find(key); it != keycodeMap.end()) {
            core.setKey(it->second, keyState);
        }
    }

    void setKeyMapping(SDL_Keycode keycode, Joypad::Key joypadKey);

    const std::map<SDL_Keycode, Joypad::Key>& getKeycodeMap() const;
    const std::map<Joypad::Key, SDL_Keycode>& getJoypadMap() const;

private:
    std::string getSavePath() const;
    std::string getStatePath() const;

    Core& core;

    struct {
        path romPath {};
        bool isLoaded {};
    } rom;

    bool paused {true};

    std::map<SDL_Keycode, Joypad::Key> keycodeMap;
    std::map<Joypad::Key, SDL_Keycode> joypadMap;

#ifdef ENABLE_DEBUGGER
    struct {
        std::unique_ptr<DebuggerBackend> backend {};
        std::unique_ptr<DebuggerFrontend> frontend {};

        struct {
            std::function<void()> onPullingCommand {};
            std::function<bool(const std::string&)> onCommandPulled {};
        } callbacks;
    } debugger;
#endif
};

#endif // CORECONTROLLER_H
