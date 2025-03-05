#include "controllers/runcontroller.h"

namespace {
constexpr uint32_t CYCLES_PER_BURST = 4;
}

#ifdef ENABLE_TWO_PLAYERS_MODE
RunController::RunController(Core& core1, Core* core2) :
    core1 {core1},
    core2 {core2 ? std::optional<CoreController> {*core2} : std::nullopt},
    two_players_mode {core2 != nullptr}
#else
CoreControllers::CoreControllers(Core& core1) :
    core1 {core1}
#endif
{
}

void RunController::frame() {
#ifdef ENABLE_TWO_PLAYERS_MODE
    if (two_players_mode) {
        bool frame_done[2] {};

        while (!frame_done[0] || !frame_done[1]) {
            frame_done[0] |= core1.run_for_cycles(CYCLES_PER_BURST);
            frame_done[1] |= core2->run_for_cycles(CYCLES_PER_BURST);
        }
    } else {
        core1.frame();
    }
#else
    core1.frame();
#endif
}

bool RunController::is_two_players_mode() const {
#ifdef ENABLE_TWO_PLAYERS_MODE
    return two_players_mode;
#else
    return false;
#endif
}

#ifdef ENABLE_TWO_PLAYERS_MODE
void RunController::attach_serial_link() {
    ASSERT(two_players_mode);
    core1.attach_serial_link(core2->get_serial_endpoint());
    core2->attach_serial_link(core1.get_serial_endpoint());
}

void RunController::detach_serial_link() {
    ASSERT(two_players_mode);
    core1.detach_serial_link();
    core2->detach_serial_link();
}

bool RunController::is_serial_link_attached() const {
    return core1.is_serial_link_attached();
}
#endif