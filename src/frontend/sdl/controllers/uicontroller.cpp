#include "controllers/uicontroller.h"

#include "docboy/core/core.h"

#include "extra/img/imgmanip.h"

#include "controllers/runcontroller.h"

#include "windows/window.h"

UiController::UiController(Window& window, RunController& runner) :
    window {window},
    runner {runner} {
}

uint32_t UiController::get_width() const {
    return window.get_width();
}

uint32_t UiController::get_height() const {
    return window.get_height();
}

void UiController::set_scaling(uint32_t scaling) {
    window.set_scaling(scaling);
}

uint32_t UiController::get_scaling() const {
    return window.get_scaling();
}

const UiController::UiAppearance& UiController::add_appearance(const LcdAppearance& rgb565, const std::string& name,
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
        rgba8888[i] = rgb565_to_rgba8888(rgb565.palette[i]);
    }

    UiAppearance a;

    const auto index = appearances.size();

    a.lcd = rgb565;
    a.menu = rgba8888;
    a.accent = rgb565_to_rgba8888(accent);
    a.name = name;
    a.index = index;

    appearances.push_back(std::move(a));

    return appearances[index];
}

const UiController::UiAppearance* UiController::get_appearance(const LcdAppearance& rgb565) const {
    for (const auto& a : appearances) {
        if (a.lcd.palette == rgb565.palette && a.lcd.default_color == rgb565.default_color) {
            return &a;
        }
    }
    return nullptr;
}

const std::vector<UiController::UiAppearance>& UiController::get_appearances() const {
    return appearances;
}

void UiController::set_current_appearance(size_t index) {
    ASSERT(index < appearances.size());
    current_appearance = appearances[index];
#ifndef ENABLE_CGB
    // TODO: CGB palettes?
    runner.get_core1().set_appearance(current_appearance.lcd);
#ifdef ENABLE_TWO_PLAYERS_MODE
    if (runner.is_two_players_mode()) {
        runner.get_core2().set_appearance(current_appearance.lcd);
    }
#endif
#endif
}

const UiController::UiAppearance& UiController::get_current_appearance() const {
    return current_appearance;
}

SDL_Renderer* UiController::get_renderer() const {
    return window.get_renderer();
}