#ifndef RUNCONTROLLER_H
#define RUNCONTROLLER_H

#include "controllers/corecontroller.h"

#include <optional>

#ifdef ENABLE_DEBUGGER
#include "docboy/debugger/backend.h"
#include "docboy/debugger/frontend.h"
#endif

class RunController {
public:
#ifdef ENABLE_TWO_PLAYERS_MODE
    RunController(Core& core1, Core* core2);
#else
    RunController(Core& core1);
#endif

    // Running/Pause
    void set_paused(bool paused_) {
        paused = paused_;
        if (paused_changed_callback) {
            paused_changed_callback(paused);
        }
    }

    bool is_paused() const {
        return paused;
    }

    void set_paused_changed_callback(std::function<void(bool)>&& callback) {
        paused_changed_callback = std::move(callback);
    }

    // Emulation
    void frame();

    // Serial
#ifdef ENABLE_TWO_PLAYERS_MODE
    void attach_serial_link();
    void detach_serial_link();
    bool is_serial_link_attached() const;
#endif

    // Mode
    bool is_two_players_mode() const;

    // Cores
    CoreController& get_core1() {
        return core1;
    }

#ifdef ENABLE_TWO_PLAYERS_MODE
    CoreController& get_core2() {
        ASSERT(core2);
        return *core2;
    }
#endif

private:
    CoreController core1;
#ifdef ENABLE_TWO_PLAYERS_MODE
    std::optional<CoreController> core2;
#endif

    bool paused {true};
    std::function<void(bool)> paused_changed_callback {};

#ifdef ENABLE_TWO_PLAYERS_MODE
    const bool two_players_mode {};
#endif
};

#endif // CORESCONTROLLER_H
