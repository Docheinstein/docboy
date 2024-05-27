#include "controloptionsscreen.h"
#include "SDL3/SDL.h"
#include "controllers/corecontroller.h"
#include "shortcutsscreen.h"

ControlOptionsScreen::ControlOptionsScreen(Context context) :
    MenuScreen(context) {

    items.a = &menu.addItem({"A", [this] {
                                 onRemapKey(Joypad::Key::A);
                             }});
    items.b = &menu.addItem({"B", [this] {
                                 onRemapKey(Joypad::Key::B);
                             }});
    items.start = &menu.addItem({"Start", [this] {
                                     onRemapKey(Joypad::Key::Start);
                                 }});
    items.select = &menu.addItem({"Select", [this] {
                                      onRemapKey(Joypad::Key::Select);
                                  }});
    items.left = &menu.addItem({"Left", [this] {
                                    onRemapKey(Joypad::Key::Left);
                                }});
    items.up = &menu.addItem({"Up", [this] {
                                  onRemapKey(Joypad::Key::Up);
                              }});
    items.right = &menu.addItem({"Right", [this] {
                                     onRemapKey(Joypad::Key::Right);
                                 }});
    items.down = &menu.addItem({"Down", [this] {
                                    onRemapKey(Joypad::Key::Down);
                                }});
}

void ControlOptionsScreen::redraw() {
    const auto getKeyName = [this](Joypad::Key joypadKey) -> std::string {
        static constexpr uint32_t MAX_LENGTH = 10;
        if (joypadKeyToRemap == joypadKey)
            return "<press>";
        std::string name = SDL_GetKeyName(core.getJoypadMap().at(joypadKey));
        if (name.size() > MAX_LENGTH)
            return name.substr(0, MAX_LENGTH - 1) + ".";
        return name;
    };

    items.a->text = "A       " + getKeyName(Joypad::Key::A);
    items.b->text = "B       " + getKeyName(Joypad::Key::B);
    items.start->text = "Start   " + getKeyName(Joypad::Key::Start);
    items.select->text = "Select  " + getKeyName(Joypad::Key::Select);
    items.left->text = "Left    " + getKeyName(Joypad::Key::Left);
    items.up->text = "Up      " + getKeyName(Joypad::Key::Up);
    items.right->text = "Right   " + getKeyName(Joypad::Key::Right);
    items.down->text = "Down    " + getKeyName(Joypad::Key::Down);

    MenuScreen::redraw();
}

void ControlOptionsScreen::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_EVENT_KEY_DOWN) {
        if (joypadKeyToRemap) {
            core.setKeyMapping(event.key.keysym.sym, *joypadKeyToRemap);
            joypadKeyToRemap = std::nullopt;
            redraw();
        } else {
            MenuScreen::handleEvent(event);
        }
    }
}

void ControlOptionsScreen::onRemapKey(Joypad::Key key) {
    joypadKeyToRemap = key;
    redraw();
}
