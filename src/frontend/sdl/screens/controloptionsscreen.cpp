#include "screens/controloptionsscreen.h"

#include "controllers/corecontroller.h"

#include "SDL3/SDL.h"

ControlOptionsScreen::ControlOptionsScreen(Context context) :
    MenuScreen {context} {

    items.a = &menu.add_item({"A", [this] {
                                  on_remap_key(Joypad::Key::A);
                              }});
    items.b = &menu.add_item({"B", [this] {
                                  on_remap_key(Joypad::Key::B);
                              }});
    items.start = &menu.add_item({"Start", [this] {
                                      on_remap_key(Joypad::Key::Start);
                                  }});
    items.select = &menu.add_item({"Select", [this] {
                                       on_remap_key(Joypad::Key::Select);
                                   }});
    items.left = &menu.add_item({"Left", [this] {
                                     on_remap_key(Joypad::Key::Left);
                                 }});
    items.up = &menu.add_item({"Up", [this] {
                                   on_remap_key(Joypad::Key::Up);
                               }});
    items.right = &menu.add_item({"Right", [this] {
                                      on_remap_key(Joypad::Key::Right);
                                  }});
    items.down = &menu.add_item({"Down", [this] {
                                     on_remap_key(Joypad::Key::Down);
                                 }});
}

void ControlOptionsScreen::redraw() {
    const auto get_key_name = [this](Joypad::Key joypad_key) -> std::string {
        static constexpr uint32_t MAX_LENGTH = 10;
        if (joypad_key_to_remap == joypad_key) {
            return "<press>";
        }
        std::string name = SDL_GetKeyName(core.get_joypad_map().at(joypad_key));
        if (name.size() > MAX_LENGTH) {
            return name.substr(0, MAX_LENGTH - 1) + ".";
        }
        return name;
    };

    items.a->text = "A       " + get_key_name(Joypad::Key::A);
    items.b->text = "B       " + get_key_name(Joypad::Key::B);
    items.start->text = "Start   " + get_key_name(Joypad::Key::Start);
    items.select->text = "Select  " + get_key_name(Joypad::Key::Select);
    items.left->text = "Left    " + get_key_name(Joypad::Key::Left);
    items.up->text = "Up      " + get_key_name(Joypad::Key::Up);
    items.right->text = "Right   " + get_key_name(Joypad::Key::Right);
    items.down->text = "Down    " + get_key_name(Joypad::Key::Down);

    MenuScreen::redraw();
}

void ControlOptionsScreen::handle_event(const SDL_Event& event) {
    if (event.type == SDL_EVENT_KEY_DOWN) {
        if (joypad_key_to_remap) {
            core.set_key_mapping(event.key.keysym.sym, *joypad_key_to_remap);
            joypad_key_to_remap = std::nullopt;
            redraw();
        } else {
            MenuScreen::handle_event(event);
        }
    }
}

void ControlOptionsScreen::on_remap_key(Joypad::Key key) {
    joypad_key_to_remap = key;
    redraw();
}
