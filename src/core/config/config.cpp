#include "config.h"

Config Config::makeDefault() {
    Config cfg;

    cfg.input.keyboardMapping[IJoypad::Key::Start] = Input::KeyboardKey::Return;
    cfg.input.keyboardMapping[IJoypad::Key::Select] = Input::KeyboardKey::Tab;
    cfg.input.keyboardMapping[IJoypad::Key::A] = Input::KeyboardKey::Z;
    cfg.input.keyboardMapping[IJoypad::Key::B] = Input::KeyboardKey::X;
    cfg.input.keyboardMapping[IJoypad::Key::Up] = Input::KeyboardKey::Up;
    cfg.input.keyboardMapping[IJoypad::Key::Down] = Input::KeyboardKey::Down;
    cfg.input.keyboardMapping[IJoypad::Key::Left] = Input::KeyboardKey::Left;
    cfg.input.keyboardMapping[IJoypad::Key::Right] = Input::KeyboardKey::Right;

    return cfg;
}
