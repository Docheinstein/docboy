#include "parser.h"
#include "utils/fileutils.h"
#include "utils/strutils.h"


class ISectionParser {
public:
    virtual ~ISectionParser() = default;

    virtual bool parse(const std::string &line, bool *ok, std::string *error) = 0;
};

class SectionParser : public ISectionParser {
public:
    explicit SectionParser(Config &config) : config(config) {}

    static std::pair<std::string, std::string> getKeyValue(const std::string &line) {
        std::vector<std::string> tokens;
        split(line, std::back_inserter(tokens), '=');

        std::vector<std::string> trimmedTokens;
        std::transform(tokens.begin(), tokens.end(), std::back_inserter(trimmedTokens),
                       [](const std::string &token) {
            return trim(token);
        });
        return std::make_pair(trimmedTokens[0], trimmedTokens[1]);
    }

    static bool stringToBool(const std::string &token) {
        if (token == "1" || token == "true" || token == "yes")
            return true;
        return false;
    }

protected:
    Config &config;
};


class InputSectionParser : public SectionParser {
public:
    explicit InputSectionParser(Config &config) : SectionParser(config),
        JOYPAD_KEYS_MAP {
            { "down", IJoypad::Key::Down },
            { "up", IJoypad::Key::Up },
            { "left", IJoypad::Key::Left },
            { "right", IJoypad::Key::Right },
            { "start", IJoypad::Key::Start },
            { "select", IJoypad::Key::Select },
            { "b", IJoypad::Key::B },
            { "a", IJoypad::Key::A },
        },
        KEYBOARD_KEYS_MAP {
        { "return", Config::Input::KeyboardKey::Return },
        { "escape", Config::Input::KeyboardKey::Escape },
        { "backspace", Config::Input::KeyboardKey::Backspace },
        { "tab", Config::Input::KeyboardKey::Tab },
        { "space", Config::Input::KeyboardKey::Space },
        { "!", Config::Input::KeyboardKey::Exclaim },
        { "\"", Config::Input::KeyboardKey::DoubleQuote },
        { "#", Config::Input::KeyboardKey::Hash },
        { "%", Config::Input::KeyboardKey::Percent },
        { "$", Config::Input::KeyboardKey::Dollar },
        { "&", Config::Input::KeyboardKey::Ampersand },
        { "'", Config::Input::KeyboardKey::Quote },
        { ")", Config::Input::KeyboardKey::LeftParen },
        { "(", Config::Input::KeyboardKey::RightParen },
        { "*", Config::Input::KeyboardKey::Asterisk },
        { "+", Config::Input::KeyboardKey::Plus },
        { ",", Config::Input::KeyboardKey::Comma },
        { "-", Config::Input::KeyboardKey::Minus },
        { ".", Config::Input::KeyboardKey::Period },
        { "/", Config::Input::KeyboardKey::Slash },
        { "0'", Config::Input::KeyboardKey::Zero },
        { "1", Config::Input::KeyboardKey::One },
        { "2", Config::Input::KeyboardKey::Two },
        { "3", Config::Input::KeyboardKey::Three },
        { "4", Config::Input::KeyboardKey::Four },
        { "5", Config::Input::KeyboardKey::Five },
        { "6", Config::Input::KeyboardKey::Six },
        { "7", Config::Input::KeyboardKey::Seven },
        { "8", Config::Input::KeyboardKey::Eight },
        { "9", Config::Input::KeyboardKey::Nine },
        { ",", Config::Input::KeyboardKey::Colon },
        { ";", Config::Input::KeyboardKey::Semicolon },
        { "<", Config::Input::KeyboardKey::Less },
        { "=", Config::Input::KeyboardKey::Equals },
        { ">", Config::Input::KeyboardKey::Greater },
        { "?", Config::Input::KeyboardKey::Question },
        { "@", Config::Input::KeyboardKey::At },
        { "[", Config::Input::KeyboardKey::LeftBracket },
        { "\\", Config::Input::KeyboardKey::Backslash },
        { "]", Config::Input::KeyboardKey::RightBracket },
        { "^", Config::Input::KeyboardKey::Caret },
        { "_", Config::Input::KeyboardKey::Underscore },
        { "`", Config::Input::KeyboardKey::Backquote },
        { "a", Config::Input::KeyboardKey::A },
        { "b", Config::Input::KeyboardKey::B },
        { "c", Config::Input::KeyboardKey::C },
        { "d", Config::Input::KeyboardKey::D },
        { "e", Config::Input::KeyboardKey::E },
        { "f", Config::Input::KeyboardKey::F },
        { "g", Config::Input::KeyboardKey::G },
        { "h", Config::Input::KeyboardKey::H },
        { "i", Config::Input::KeyboardKey::I },
        { "j", Config::Input::KeyboardKey::J },
        { "k", Config::Input::KeyboardKey::K },
        { "l", Config::Input::KeyboardKey::L },
        { "m", Config::Input::KeyboardKey::M },
        { "n", Config::Input::KeyboardKey::N },
        { "o", Config::Input::KeyboardKey::O },
        { "p", Config::Input::KeyboardKey::P },
        { "q", Config::Input::KeyboardKey::Q },
        { "r", Config::Input::KeyboardKey::R },
        { "s", Config::Input::KeyboardKey::S },
        { "t", Config::Input::KeyboardKey::T },
        { "u", Config::Input::KeyboardKey::U },
        { "v", Config::Input::KeyboardKey::V },
        { "w", Config::Input::KeyboardKey::W },
        { "x", Config::Input::KeyboardKey::X },
        { "y", Config::Input::KeyboardKey::Y },
        { "z", Config::Input::KeyboardKey::Z },
        { "capslock", Config::Input::KeyboardKey::CapsLock },
        { "f1", Config::Input::KeyboardKey::F1 },
        { "f2", Config::Input::KeyboardKey::F2 },
        { "f3", Config::Input::KeyboardKey::F3 },
        { "f4", Config::Input::KeyboardKey::F4 },
        { "f5", Config::Input::KeyboardKey::F5 },
        { "f6", Config::Input::KeyboardKey::F6 },
        { "f7", Config::Input::KeyboardKey::F7 },
        { "f8", Config::Input::KeyboardKey::F8 },
        { "f9", Config::Input::KeyboardKey::F9 },
        { "f10", Config::Input::KeyboardKey::F10 },
        { "f11", Config::Input::KeyboardKey::F11 },
        { "f12", Config::Input::KeyboardKey::F12 },
        { "print", Config::Input::KeyboardKey::Print },
        { "scrolllock", Config::Input::KeyboardKey::ScrollLock },
        { "pause", Config::Input::KeyboardKey::Pause },
        { "insert", Config::Input::KeyboardKey::Insert },
        { "home", Config::Input::KeyboardKey::Home },
        { "pageup", Config::Input::KeyboardKey::PageUp },
        { "delete", Config::Input::KeyboardKey::Delete },
        { "end", Config::Input::KeyboardKey::End },
        { "pagedown", Config::Input::KeyboardKey::PageDown },
        { "right", Config::Input::KeyboardKey::Right },
        { "left", Config::Input::KeyboardKey::Left },
        { "down", Config::Input::KeyboardKey::Down },
        { "up", Config::Input::KeyboardKey::Up },
    } {

    }

    bool parse(const std::string &line, bool *ok, std::string *error) override {
        const auto [key, value] = getKeyValue(line);

        auto joypadKey = JOYPAD_KEYS_MAP.find(key);
        auto keyboardKey = KEYBOARD_KEYS_MAP.find(value);
        if (joypadKey == JOYPAD_KEYS_MAP.end()) {
            if (ok)
                *ok = false;
            if (error)
                *error = "unknown joypad key: '" + key + " ' ";
            return false;
        }
        if (keyboardKey == KEYBOARD_KEYS_MAP.end()) {
            if (ok)
                *ok = false;
            if (error)
                *error = "unknown keyboard key: '" + key + " ' ";
            return false;
        }

        config.input.keyboardMapping[joypadKey->second] = keyboardKey->second;

        return true;
    }

private:
    std::map<std::string, IJoypad::Key> JOYPAD_KEYS_MAP;
    std::map<std::string, Config::Input::KeyboardKey> KEYBOARD_KEYS_MAP;
};


class DebugSectionParser : public SectionParser {
public:
    explicit DebugSectionParser(Config &config) : SectionParser(config) {}
    bool parse(const std::string &line, bool *ok, std::string *error) override {
        const auto [key, value] = getKeyValue(line);

        if (key == "cpu")
            config.debug.sections.cpu = stringToBool(value);
        else if (key == "ppu")
            config.debug.sections.ppu = stringToBool(value);
        else if (key == "flags")
            config.debug.sections.flags = stringToBool(value);
        else if (key == "registers")
            config.debug.sections.registers = stringToBool(value);
        else if (key == "interrupts")
            config.debug.sections.interrupts = stringToBool(value);
        else if (key == "joypad")
            config.debug.sections.io.joypad = stringToBool(value);
        else if (key == "serial")
            config.debug.sections.io.serial = stringToBool(value);
        else if (key == "timers")
            config.debug.sections.io.timers = stringToBool(value);
        else if (key == "sound")
            config.debug.sections.io.sound = stringToBool(value);
        else if (key == "lcd")
            config.debug.sections.io.lcd = stringToBool(value);
        else if (key == "code")
            config.debug.sections.code = stringToBool(value);
        else {
            if (ok)
                *ok = false;
            if (error)
                *error = "unknown section: '" + key + " ' ";
            return false;
        }

        return true;
    }
};

ConfigParser::ConfigParser() : sectionParser() {

}

// must be defined because a member of the class is a std::unique_ptr<T>
// where T is forward declared
ConfigParser::~ConfigParser() = default;

Config ConfigParser::parse(const std::string &filename, bool *ok, std::string *error) {
    Config config = Config::makeDefault();

    std::vector<std::string> lines = read_file_lines(filename, ok);
    if (ok && !(*ok)) {
        if (error)
            *error = "failed to read file";
        return config;
    }

    for (const auto &line : lines) {
        if (equals_ignore_case(line, "[input]")) {
            sectionParser = std::make_unique<InputSectionParser>(config);
        } else if (equals_ignore_case(line, "[debug]")) {
            sectionParser = std::make_unique<DebugSectionParser>(config);
        } else if (sectionParser) {
            if (!sectionParser->parse(line, ok, error))
                break;
        }
    }
    return config;
}
