#include "controllers/uicontroller.h"

#include "docboy/core/core.h"

#include "extra/img/imgmanip.h"

#include "windows/window.h"

UiController::UiController(Window& window, Core& core) :
    window {window},
    core {core} {
}

void UiController::set_scaling(uint32_t scaling) {
    window.set_scaling(scaling);
}

uint32_t UiController::get_scaling() const {
    return window.get_scaling();
}

const UiController::Palette& UiController::add_palette(const std::array<uint16_t, 4>& rgb565, const std::string& name,
                                                       std::optional<uint16_t> optional_accent) {
    const auto rgb565_to_rgba8888 = [](uint16_t p) {
        return convert_pixel(p, ImageFormat::RGB565, ImageFormat::RGB888) << 8 | 0x000000FF;
    };

    uint16_t accent;
    if (optional_accent) {
        accent = *optional_accent;
    } else {
        // TODO: automatically compute an high contrast value
        accent = 0xF800;
    }

    std::array<uint32_t, 4> rgba8888 {};

    for (uint32_t i = 0; i < 4; i++) {
        rgba8888[i] = rgb565_to_rgba8888(rgb565[i]);
    }

    Palette p;

    const auto index = palettes.size();

    p.rgb565.palette = rgb565;
    p.rgb565.accent = accent;
    p.rgba8888.palette = rgba8888;
    p.rgba8888.accent = rgb565_to_rgba8888(accent);
    p.name = name;
    p.index = index;

    palettes.push_back(std::move(p));

    return palettes[index];
}

const UiController::Palette* UiController::get_palette(const std::array<uint16_t, 4>& rgb565) const {
    for (const auto& p : palettes) {
        if (p.rgb565.palette == rgb565) {
            return &p;
        }
    }
    return nullptr;
}

const std::vector<UiController::Palette>& UiController::get_palettes() const {
    return palettes;
}

void UiController::set_current_palette(size_t index) {
    ASSERT(index < palettes.size());
    current_palette = palettes[index];
#ifndef ENABLE_CGB
    // TODO: CGB palettes?
    core.gb.lcd.set_palette(
        std::vector<uint16_t> {current_palette.rgb565.palette.begin(), current_palette.rgb565.palette.end()});
#endif
}

const UiController::Palette& UiController::get_current_palette() const {
    return current_palette;
}

SDL_Renderer* UiController::get_renderer() const {
    return window.get_renderer();
}
