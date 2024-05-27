#include "debuggercontroller.h"
#include "corecontroller.h"
#include "window.h"

DebuggerController::DebuggerController(Window& window, Core& core, CoreController& coreController) :
    window(window),
    core(core),
    coreController(coreController) {
}

bool DebuggerController::isDebuggerAttached() const {
    return coreController.isDebuggerAttached();
}

bool DebuggerController::attachDebugger(bool proceedExecution) {
    // Highlight the next pixel when the execution is stopped from the debugger
    const auto onPullingCommand = [this]() {
        // Cache the next pixel color
        Lcd::PixelRgb565& nextPixel = core.gb.lcd.getPixels()[core.gb.lcd.getCursor()];
        const Lcd::PixelRgb565 nextPixelColor = nextPixel;

        // Mark the current dot as a white pixel (useful for debug PPU)
        nextPixel = 0xFFFF;

        // Render framebuffer
        window.render();

        // Restore original color
        nextPixel = nextPixelColor;
    };

    const auto onCommandPulled = [this](const std::string& cmd) {
        if (cmd == "clear") {
            memset(core.gb.lcd.getPixels(), 0, Lcd::PIXEL_BUFFER_SIZE);
            window.clear();
            return true;
        }
        return false;
    };

    return coreController.attachDebugger(onPullingCommand, onCommandPulled, proceedExecution);
}

bool DebuggerController::detachDebugger() {
    return coreController.detachDebugger();
}
