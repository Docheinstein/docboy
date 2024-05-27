#include "uicontroller.h"
#include "docboy/core/core.h"
#include "extra/img/imgmanip.h"
#include "window.h"

UiController::UiController(Window& window, Core& core) :
    window(window),
    core(core) {
}

void UiController::setScaling(uint32_t scaling) {
    window.setScaling(scaling);
}

uint32_t UiController::getScaling() const {
    return window.getScaling();
}

const UiController::Palette& UiController::addPalette(const std::array<uint16_t, 4>& rgb565, const std::string& name,
                                                      std::optional<uint16_t> optionalAccent) {
    const auto rgb565toRgba8888 = [](uint16_t p) {
        return convert_pixel(p, ImageFormat::RGB565, ImageFormat::RGB888) << 8 | 0x000000FF;
    };

    uint16_t accent;
    if (optionalAccent) {
        accent = *optionalAccent;
    } else {
        // TODO: automatically compute an high contrast value
        accent = 0xF800;
    }

    std::array<uint32_t, 4> rgba8888 {};

    for (uint32_t i = 0; i < 4; i++)
        rgba8888[i] = rgb565toRgba8888(rgb565[i]);

    Palette p;

    const auto index = palettes.size();

    p.rgb565.palette = rgb565;
    p.rgb565.accent = accent;
    p.rgba8888.palette = rgba8888;
    p.rgba8888.accent = rgb565toRgba8888(accent);
    p.name = name;
    p.index = index;

    palettes.push_back(std::move(p));

    return palettes[index];
}

const UiController::Palette* UiController::getPalette(const std::array<uint16_t, 4>& rgb565) const {
    for (const auto& p : palettes) {
        if (p.rgb565.palette == rgb565) {
            return &p;
        }
    }
    return nullptr;
}

const std::vector<UiController::Palette>& UiController::getPalettes() const {
    return palettes;
}

void UiController::setCurrentPalette(size_t index) {
    check(index < palettes.size());
    currentPalette = palettes[index];
    core.gb.lcd.setPalette(currentPalette.rgb565.palette);
}

const UiController::Palette& UiController::getCurrentPalette() const {
    return currentPalette;
}

SDL_Renderer* UiController::getRenderer() const {
    return window.getRenderer();
}
