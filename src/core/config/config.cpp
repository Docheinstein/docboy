#include "config.h"

Config Config::makeDefault() {
    return {.input = {.keyboardMapping = {{IJoypad::Key::Start, Input::KeyboardKey::Return},
                                          {IJoypad::Key::Select, Input::KeyboardKey::Tab},
                                          {IJoypad::Key::A, Input::KeyboardKey::Z},
                                          {IJoypad::Key::B, Input::KeyboardKey::X},
                                          {IJoypad::Key::Up, Input::KeyboardKey::Up},
                                          {IJoypad::Key::Down, Input::KeyboardKey::Down},
                                          {IJoypad::Key::Left, Input::KeyboardKey::Left},
                                          {IJoypad::Key::Right, Input::KeyboardKey::Right}}},
            .debug = {.sections = {.breakpoints = true,
                                   .watchpoints = true,
                                   .cpu = true,
                                   .ppu = true,
                                   .flags = true,
                                   .registers = true,
                                   .interrupts = true,
                                   .io{
                                       .joypad = true,
                                       .serial = true,
                                       .timers = true,
                                       .sound = true,
                                       .lcd = true,
                                   },
                                   .code = true}}};
}
