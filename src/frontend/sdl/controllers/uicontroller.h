#ifndef UICONTROLLER_H
#define UICONTROLLER_H

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "docboy/lcd/appearance.h"

class Window;
class Core;
struct SDL_Renderer;

class RunController;

class UiController {
public:
#ifdef ENABLE_CGB
    struct LcdAppearance {
        PixelRgb565 default_color {};
        std::array<PixelRgb565, 4> palette {};
    };
#else
    using LcdAppearance = Appearance;
#endif

    using MenuAppearance = std::array<uint32_t, 4>;

    struct UiAppearance {
        LcdAppearance lcd {};
        MenuAppearance menu {};

        uint32_t accent {};

        std::string name {};
        std::size_t index {};
    };

    explicit UiController(Window& window, RunController& runner);

    // Size
    uint32_t get_width() const;
    uint32_t get_height() const;

    // Scaling
    void set_scaling(uint32_t scaling);
    uint32_t get_scaling() const;

    // Palettes
    const UiAppearance& add_appearance(const LcdAppearance& rgb565, const std::string& name,
                                       std::optional<PixelRgb565> optional_accent = std::nullopt);
    const UiAppearance* get_appearance(const LcdAppearance& rgb565) const;
    const std::vector<UiAppearance>& get_appearances() const;

    void set_current_appearance(size_t index);
    const UiAppearance& get_current_appearance() const;

    // SDL
    SDL_Renderer* get_renderer() const;

private:
    Window& window;
    RunController& runner;

    UiAppearance current_appearance {};
    std::vector<UiAppearance> appearances;
};

#endif // UICONTROLLER_H
