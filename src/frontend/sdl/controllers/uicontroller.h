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
    void setScaling(uint32_t scaling);
    uint32_t getScaling() const;

    // Palettes
    const Palette& addPalette(const std::array<uint16_t, 4>& rgb565, const std::string& name,
                              std::optional<uint16_t> accent = std::nullopt);
    const Palette* getPalette(const std::array<uint16_t, 4>& rgb565) const;
    const std::vector<Palette>& getPalettes() const;

    void setCurrentPalette(size_t index);
    const Palette& getCurrentPalette() const;

    // SDL
    SDL_Renderer* getRenderer() const;

private:
    Window& window;
    Core& core;

    Palette currentPalette {};
    std::vector<Palette> palettes;
};

#endif // UICONTROLLER_H
