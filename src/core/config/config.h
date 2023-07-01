#ifndef CONFIG_H
#define CONFIG_H

#include <map>
#include "core/joypad/joypad.h"

struct Config {
public:
    struct Input {
        enum class KeyboardKey {
            Return,
            Escape,
            Backspace,
            Tab,
            Space,
            Exclaim,
            DoubleQuote,
            Hash,
            Percent,
            Dollar,
            Ampersand,
            Quote,
            LeftParen,
            RightParen,
            Asterisk,
            Plus,
            Comma,
            Minus,
            Period,
            Slash,
            Zero,
            One,
            Two,
            Three,
            Four,
            Five,
            Six,
            Seven,
            Eight,
            Nine,
            Colon,
            Semicolon,
            Less,
            Equals,
            Greater,
            Question,
            At,
            LeftBracket,
            Backslash,
            RightBracket,
            Caret,
            Underscore,
            Backquote,
            A,
            B,
            C,
            D,
            E,
            F,
            G,
            H,
            I,
            J,
            K,
            L,
            M,
            N,
            O,
            P,
            Q,
            R,
            S,
            T,
            U,
            V,
            W,
            X,
            Y,
            Z,
            CapsLock,
            F1,
            F2,
            F3,
            F4,
            F5,
            F6,
            F7,
            F8,
            F9,
            F10,
            F11,
            F12,
            Print,
            ScrollLock,
            Pause,
            Insert,
            Home,
            PageUp,
            Delete,
            End,
            PageDown,
            Right,
            Left,
            Down,
            Up
        };
        std::map<IJoypad::Key, KeyboardKey> keyboardMapping;
    } input;

    struct Debug {
        struct {
            bool breakpoints;
            bool watchpoints;
            bool cpu;
            bool ppu;
            bool flags;
            bool registers;
            bool interrupts;
            struct {
                bool joypad;
                bool serial;
                bool timers;
                bool sound;
                bool lcd;
            } io;
            bool code;
        } sections;
    } debug;

    static Config makeDefault();
};
#endif // CONFIG_H