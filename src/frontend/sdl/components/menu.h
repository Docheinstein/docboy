#ifndef MENU_H
#define MENU_H

#include <array>
#include <functional>
#include <string>
#include <utility>

#include "components/text.h"

#include "utils/textures.h"
#include "utils/vector.h"

#include "SDL3/SDL_keycode.h"

struct SDL_Texture;

struct MenuItem {
    MenuItem() = default;
    explicit MenuItem(std::string&& t) :
        text {std::move(t)} {
    }

    std::string text {};
    std::function<void()> on_enter_fn {};
    std::function<void()> on_prev_fn {};
    std::function<void()> on_next_fn {};

    MenuItem&& on_enter(const std::function<void()>& f) && {
        on_enter_fn = f;
        return std::move(*this);
    }

    MenuItem&& on_change(const std::function<void()>& f) && {
        on_prev_fn = f;
        on_next_fn = f;
        return std::move(*this);
    }

    MenuItem&& on_prev(const std::function<void()>& f) && {
        on_prev_fn = f;
        return std::move(*this);
    }

    MenuItem&& on_next(const std::function<void()>& f) && {
        on_next_fn = f;
        return std::move(*this);
    }
};

class Menu {
public:
    explicit Menu(SDL_Texture* texture, const std::array<uint32_t, 4>& palette);

    template <typename... Args>
    MenuItem& add_item(Args... args) {
        items.emplace_back(std::forward<Args...>(args...));
        return items[items.size() - 1];
    }

    void set_navigation_enabled(bool enabled);

    void redraw();

    void handle_input(SDL_Keycode key);

private:
    SDL_Texture* texture {};
    int height {};
    int width {};

    const std::array<uint32_t, 4>& palette;

    Vector<MenuItem, 16> items;
    uint8_t cursor {};

    bool navigation {true};
};

#endif // MENU_H
