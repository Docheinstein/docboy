#ifndef UICONTROLLER_H
#define UICONTROLLER_H

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class Window;
class Core;
struct SDL_Renderer;

class UiController {
public:
    struct Palette {
        struct {
            std::array<uint16_t, 4> palette {};
            uint16_t accent {};
        } rgb565;
        struct {
            std::array<uint32_t, 4> palette {};
            uint32_t accent {};
        } rgba8888;
        std::string name {};
        std::size_t index {};
    };

    explicit UiController(Window& window, Core& core);

    // Scaling
    void set_scaling(uint32_t scaling);
    uint32_t get_scaling() const;

    // Palettes
    const Palette& add_palette(const std::array<uint16_t, 4>& rgb565, const std::string& name,
                               std::optional<uint16_t> optional_accent = std::nullopt);
    const Palette* get_palette(const std::array<uint16_t, 4>& rgb565) const;
    const std::vector<Palette>& get_palettes() const;

    void set_current_palette(size_t index);
    const Palette& get_current_palette() const;

    // SDL
    SDL_Renderer* get_renderer() const;

private:
    Window& window;
    Core& core;

    Palette current_palette {};
    std::vector<Palette> palettes;
};

#endif // UICONTROLLER_H
