#include "controllers/debuggercontroller.h"

#include "controllers/corecontroller.h"
#include "windows/window.h"

DebuggerController::DebuggerController(Window& window, Core& core, CoreController& core_controller) :
    window {window},
    core {core},
    core_controller {core_controller} {
}

bool DebuggerController::is_debugger_attached() const {
    return core_controller.is_debugger_attached();
}

bool DebuggerController::attach_debugger(bool proceed_execution) {
    // Highlight the next pixel when the execution is stopped from the debugger
    const auto on_pulling_command = [this]() {
        // Cache the next pixel color
        Lcd::PixelRgb565& next_pixel = core.gb.lcd.get_pixels()[core.gb.lcd.get_cursor()];
        const Lcd::PixelRgb565 next_pixel_color = next_pixel;

        // Mark the current dot as a special pixel (useful for debug PPU)
        next_pixel = 0xF800;

        // Render framebuffer
        window.render();

        // Restore original color
        next_pixel = next_pixel_color;
    };

    const auto on_command_pulled = [this](const std::string& cmd) {
        if (cmd == "clear") {
            memset(core.gb.lcd.get_pixels(), 0, Lcd::PIXEL_BUFFER_SIZE);
            window.clear();
            return true;
        }
        return false;
    };

    return core_controller.attach_debugger(on_pulling_command, on_command_pulled, proceed_execution);
}

bool DebuggerController::detach_debugger() {
    return core_controller.detach_debugger();
}
