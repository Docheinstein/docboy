#include <chrono>
#include <iostream>
#include <map>

#include "SDL3/SDL.h"

#include "docboy/cartridge/factory.h"

#include "controllers/corecontroller.h"
#include "controllers/maincontroller.h"
#include "controllers/uicontroller.h"
#include "screens/gamescreen.h"
#include "screens/launcherscreen.h"
#include "windows/window.h"

#include "extra/cartridge/header.h"
#include "extra/img/imgmanip.h"
#include "extra/ini/reader/reader.h"
#include "extra/ini/writer/writer.h"
#include "extra/serial/endpoints/console.h"

#include "args/args.h"

#include "utils/formatters.h"
#include "utils/os.h"
#include "utils/strings.h"

#ifdef NFD
#include "nfd.h"
#endif

#ifdef ENABLE_BOOTROM
#include "docboy/bootrom/factory.h"
#endif

#ifdef ENABLE_DEBUGGER
#include "controllers/debuggercontroller.h"
#include "docboy/debugger/backend.h"
#include "docboy/debugger/frontend.h"
#endif

namespace {

struct Preferences {
    Lcd::Palette palette {};
    struct {
        SDL_Keycode a {};
        SDL_Keycode b {};
        SDL_Keycode start {};
        SDL_Keycode select {};
        SDL_Keycode left {};
        SDL_Keycode up {};
        SDL_Keycode right {};
        SDL_Keycode down {};
    } keys {};
    uint32_t scaling {};
    int x {};
    int y {};
};

Preferences make_default_preferences() {
    Preferences prefs {};
    prefs.palette = Lcd::DEFAULT_PALETTE;
    prefs.keys.a = SDLK_z;
    prefs.keys.b = SDLK_x;
    prefs.keys.start = SDLK_RETURN;
    prefs.keys.select = SDLK_TAB;
    prefs.keys.left = SDLK_LEFT;
    prefs.keys.up = SDLK_UP;
    prefs.keys.right = SDLK_RIGHT;
    prefs.keys.down = SDLK_DOWN;
    prefs.scaling = 2;
    prefs.x = 200;
    prefs.y = 200;
    return prefs;
}

void dump_cartridge_info(const ICartridge& cartridge) {
    const auto header = CartridgeHeader::parse(cartridge);
    std::cout << "Title             :  " << header.title_as_string() << "\n";
    std::cout << "Cartridge type    :  " << hex(header.cartridge_type) << "     ("
              << header.cartridge_type_description() << ")\n";
    std::cout << "Licensee (new)    :  " << hex(header.new_licensee_code) << "  ("
              << header.new_licensee_code_description() << ")\n";
    std::cout << "Licensee (old)    :  " << hex(header.old_licensee_code) << "     ("
              << header.old_licensee_code_description() << ")\n";
    std::cout << "ROM Size          :  " << hex(header.rom_size) << "     (" << header.rom_size_description() << ")\n";
    std::cout << "RAM Size          :  " << hex(header.ram_size) << "     (" << header.ram_size_description() << ")\n";
    std::cout << "CGB flag          :  " << hex(header.cgb_flag) << "     (" << header.cgb_flag_description() << ")\n";
    std::cout << "SGB flag          :  " << hex(header.sgb_flag) << "\n";
    std::cout << "Destination Code  :  " << hex(header.destination_code) << "\n";
    std::cout << "Rom Version Num.  :  " << hex(header.rom_version_number) << "\n";
    std::cout << "Header checksum   :  " << hex(header.header_checksum) << "\n";
}

template <typename T>
std::optional<T> parse_int(const std::string& s) {
    const char* cstr = s.c_str();
    char* endptr {};
    errno = 0;

    T val = std::strtol(cstr, &endptr, 10);

    if (errno || endptr == cstr || *endptr != '\0') {
        return std::nullopt;
    }

    return val;
}

std::optional<uint16_t> parse_hex_uint16(const std::string& s) {
    const char* cstr = s.c_str();
    char* endptr {};
    errno = 0;

    uint16_t val = std::strtol(cstr, &endptr, 16);

    if (errno || endptr == cstr || *endptr != '\0') {
        return std::nullopt;
    }

    return val;
}

std::optional<SDL_Keycode> parse_keycode(const std::string& s) {
    SDL_Keycode keycode = SDL_GetKeyFromName(s.c_str());
    return keycode != SDLK_UNKNOWN ? std::optional {keycode} : std::nullopt;
}

template <uint8_t Size>
std::optional<std::array<uint16_t, Size>> parse_hex_array(const std::string& s) {
    std::vector<std::string> tokens;
    split(s, std::back_inserter(tokens), [](char ch) {
        return ch == ',';
    });

    if (tokens.size() != Size) {
        return std::nullopt;
    }

    std::array<uint16_t, Size> ret {};

    for (uint32_t i = 0; i < Size; i++) {
        if (auto u = parse_hex_uint16(tokens[i])) {
            ret[i] = *u;
        } else {
            return std::nullopt;
        }
    }

    return ret;
}

void ensure_file_exists(const std::string& path) {
    if (!file_exists(path)) {
        std::cerr << "ERROR: failed to load '" << path << "'" << std::endl;
        exit(1);
    }
}

std::string get_preferences_path() {
    char* pref_path_cstr = SDL_GetPrefPath("", "DocBoy");
    std::string pref_path = (Path {pref_path_cstr} / "prefs.ini").string();
    free(pref_path_cstr);
    return pref_path;
}

void read_preferences(const std::string& path, Preferences& p) {
    IniReader ini_reader;
    ini_reader.add_comment_prefix("#");
    ini_reader.add_property("dmg_palette", p.palette, parse_hex_array<4>);
    ini_reader.add_property("a", p.keys.a, parse_keycode);
    ini_reader.add_property("b", p.keys.b, parse_keycode);
    ini_reader.add_property("start", p.keys.start, parse_keycode);
    ini_reader.add_property("select", p.keys.select, parse_keycode);
    ini_reader.add_property("left", p.keys.left, parse_keycode);
    ini_reader.add_property("up", p.keys.up, parse_keycode);
    ini_reader.add_property("right", p.keys.right, parse_keycode);
    ini_reader.add_property("down", p.keys.down, parse_keycode);
    ini_reader.add_property("scaling", p.scaling, parse_int<uint32_t>);
    ini_reader.add_property("x", p.x, parse_int<int32_t>);
    ini_reader.add_property("y", p.y, parse_int<int32_t>);

    const auto result = ini_reader.parse(path);
    switch (result.outcome) {
    case IniReader::Result::Outcome::Success:
        break;
    case IniReader::Result::Outcome::ErrorReadFailed:
        std::cerr << "ERROR: failed to read '" << path << "'" << std::endl;
        exit(2);
    case IniReader::Result::Outcome::ErrorParseFailed:
        std::cerr << "ERROR: failed to parse  '" << path << "': error at line " << result.last_read_line << std::endl;
        exit(2);
    }
}

void write_preferences(const std::string& path, const Preferences& p) {
    std::map<std::string, std::string> properties;

    properties.emplace("dmg_palette", join(p.palette, ",", [](uint16_t val) {
                           return hex(val);
                       }));
    properties.emplace("a", SDL_GetKeyName(p.keys.a));
    properties.emplace("b", SDL_GetKeyName(p.keys.b));
    properties.emplace("start", SDL_GetKeyName(p.keys.start));
    properties.emplace("select", SDL_GetKeyName(p.keys.select));
    properties.emplace("left", SDL_GetKeyName(p.keys.left));
    properties.emplace("up", SDL_GetKeyName(p.keys.up));
    properties.emplace("right", SDL_GetKeyName(p.keys.right));
    properties.emplace("down", SDL_GetKeyName(p.keys.down));
    properties.emplace("scaling", std::to_string(p.scaling));
    properties.emplace("x", std::to_string(p.x));
    properties.emplace("y", std::to_string(p.y));

    IniWriter ini_writer;
    if (!ini_writer.write(properties, path)) {
        std::cerr << "WARN: failed to write '" << path << "'" << std::endl;
    }
}
} // namespace

int main(int argc, char* argv[]) {
    // Parse command line arguments
    struct {
        std::string rom;
#ifdef ENABLE_BOOTROM
        std::string boot_rom;
#endif
        std::string config {};
        bool serial {};
        uint32_t scaling {};
        bool dump_cartridge_info {};
#ifdef ENABLE_DEBUGGER
        bool attach_debugger {};
#endif
    } args;

    Args::Parser args_parser {};
#ifdef ENABLE_BOOTROM
    args_parser.add_argument(args.boot_rom, "boot-rom").help("Boot ROM");
#endif
    args_parser.add_argument(args.rom, "rom").required(false).help("ROM");
    args_parser.add_argument(args.config, "--config", "-c").help("Read configuration file");
#ifdef ENABLE_SERIAL
    args_parser.add_argument(args.serial, "--serial", "-s").help("Display serial console");
#endif
    args_parser.add_argument(args.scaling, "--scaling", "-z").help("Scaling factor");
    args_parser.add_argument(args.dump_cartridge_info, "--cartridge-info", "-i").help("Dump cartridge info and quit");
#ifdef ENABLE_DEBUGGER
    args_parser.add_argument(args.attach_debugger, "--debugger", "-d").help("Attach debugger");
#endif

    // Parse command line arguments
    if (!args_parser.parse(argc, argv, 1)) {
        return 1;
    }

    // Eventually just dump cartridge info and quit
    if (!args.rom.empty() && args.dump_cartridge_info) {
        dump_cartridge_info(*CartridgeFactory::create(args.rom));
        return 0;
    }

    // Create default preferences
    Preferences prefs {make_default_preferences()};

    // Eventually load preferences
    std::string pref_path {get_preferences_path()};
    if (file_exists(pref_path)) {
        read_preferences(pref_path, prefs);
    }

    // Eventually load configuration file (override preferences)
    if (!args.config.empty()) {
        ensure_file_exists(args.config);
        read_preferences(args.config, prefs);
    }

    // Args override both preferences and config
    if (args.scaling > 0) {
        prefs.scaling = args.scaling;
    }

    prefs.scaling = std::max(prefs.scaling, 1U);

#ifdef ENABLE_BOOTROM
    ensure_file_exists(args.boot_rom);
    if (file_size(args.boot_rom) != BootRom::Size) {
        std::cerr << "ERROR: invalid boot rom '" << args.boot_rom << "'" << std::endl;
        return 3;
    }
#endif

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "ERROR: SDL initialization failed '" << SDL_GetError() << "'" << std::endl;
        return 4;
    }

#ifdef NFD
    // Initialize NFD
    if (NFD_Init() != NFD_OKAY) {
        std::cerr << "ERROR: NFD initialization failed '" << NFD_GetError() << "'" << std::endl;
        return 5;
    }
#endif

    // Create GameBoy and Core
#ifdef ENABLE_BOOTROM
    auto gb {std::make_unique<GameBoy>(BootRomFactory::create(args.boot_rom))};
#else
    auto gb {std::make_unique<GameBoy>()};
#endif

    Core core {*gb};

    // Create SDL window
    Window window {prefs.x, prefs.y, prefs.scaling};

    // Create screens' context
    CoreController core_controller {core};
    UiController ui_controller {window, core};
    NavController nav_controller {window.get_screen_stack()};
    MainController main_controller {};
#ifdef ENABLE_DEBUGGER
    DebuggerController debugger_controller {window, core, core_controller};
#endif

#ifdef ENABLE_DEBUGGER
    Screen::Controllers controllers {core_controller, ui_controller, nav_controller, main_controller,
                                     debugger_controller};
#else
    Screen::Controllers controllers {core_controller, ui_controller, nav_controller, main_controller};
#endif

    Screen::Context context {controllers, {0xFF}};

    // Setup context accordingly to preferences

    // - Keymap
    core_controller.set_key_mapping(prefs.keys.a, Joypad::Key::A);
    core_controller.set_key_mapping(prefs.keys.b, Joypad::Key::B);
    core_controller.set_key_mapping(prefs.keys.start, Joypad::Key::Start);
    core_controller.set_key_mapping(prefs.keys.select, Joypad::Key::Select);
    core_controller.set_key_mapping(prefs.keys.left, Joypad::Key::Left);
    core_controller.set_key_mapping(prefs.keys.up, Joypad::Key::Up);
    core_controller.set_key_mapping(prefs.keys.right, Joypad::Key::Right);
    core_controller.set_key_mapping(prefs.keys.down, Joypad::Key::Down);

    // - Scaling
    ui_controller.set_scaling(prefs.scaling);

    // - Palette
    ui_controller.add_palette({0x84A0, 0x4B40, 0x2AA0, 0x1200}, "Green", 0xFFFF);
    ui_controller.add_palette({0xFFFF, 0xAD55, 0x52AA, 0x0000}, "Grey", 0xA901);
    const auto* palette = ui_controller.get_palette(prefs.palette);
    if (!palette) {
        // No know palette exists for this configuration: add a new one
        palette = &ui_controller.add_palette(prefs.palette, "User");
    }
    ui_controller.set_current_palette(palette->index);

#ifdef ENABLE_SERIAL
    // Eventually attach serial
    std::unique_ptr<SerialConsole> serial_console;
    std::unique_ptr<SerialLink> serial_link;
    if (args.serial) {
        serial_console = std::make_unique<SerialConsole>(std::cerr, 16);
        serial_link = std::make_unique<SerialLink>();
        serial_link->plug1.attach(*serial_console);
        core.attach_serial_link(serial_link->plug2);
    }
#endif

#ifdef ENABLE_DEBUGGER
    // Eventually attach debugger
    if (args.attach_debugger) {
        debugger_controller.attach_debugger();
    }
#endif

    if (!args.rom.empty()) {
        // Start with loaded game
        core_controller.load_rom(args.rom);
        nav_controller.push(std::make_unique<GameScreen>(context));
    } else {
        // Start with launcher screen instead
        nav_controller.push(std::make_unique<LauncherScreen>(context));
    }

    // Main loop
    SDL_Event e;
    std::chrono::high_resolution_clock::time_point next_frame_time = std::chrono::high_resolution_clock::now();

#ifdef ENABLE_DEBUGGER
    while (!main_controller.should_quit() && !core.is_asking_to_quit()) {
#else
    while (!main_controller.should_quit()) {
#endif
        // Wait until next frame
        while (std::chrono::high_resolution_clock::now() < next_frame_time) {
        }
        next_frame_time += main_controller.get_frame_time();

        // Handle events
        while (SDL_PollEvent(&e) != 0) {
            switch (e.type) {
            case SDL_EVENT_QUIT:
                main_controller.quit();
                break;
            default:
                window.handle_event(e);
            }
        }

        // Eventually advance emulation by one frame
        if (!core_controller.is_paused()) {
            core_controller.frame();
        }

        // Render current frame
        window.render();
    }

    // Write RAM state
    if (core_controller.is_rom_loaded()) {
        core_controller.write_save();
    }

    // Write current preferences
    prefs.palette = ui_controller.get_current_palette().rgb565.palette;
    prefs.scaling = window.get_scaling();

    const auto& keycodes {core_controller.get_joypad_map()};
    prefs.keys.a = keycodes.at(Joypad::Key::A);
    prefs.keys.b = keycodes.at(Joypad::Key::B);
    prefs.keys.start = keycodes.at(Joypad::Key::Start);
    prefs.keys.select = keycodes.at(Joypad::Key::Select);
    prefs.keys.left = keycodes.at(Joypad::Key::Left);
    prefs.keys.up = keycodes.at(Joypad::Key::Up);
    prefs.keys.right = keycodes.at(Joypad::Key::Right);
    prefs.keys.down = keycodes.at(Joypad::Key::Down);

    Window::Position pos = window.get_position();
    prefs.x = pos.x;
    prefs.y = pos.y;

    write_preferences(pref_path, prefs);

#ifdef ENABLE_SERIAL
    if (serial_console) {
        serial_console->flush();
    }
#endif

    return 0;
}