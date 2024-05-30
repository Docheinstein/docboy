#include "SDL3/SDL.h"
#include "args/args.h"
#include "controllers/corecontroller.h"
#include "controllers/maincontroller.h"
#include "controllers/uicontroller.h"
#include "docboy/cartridge/factory.h"
#include "extra/cartridge/header.h"
#include "extra/img/imgmanip.h"
#include "extra/ini/reader/reader.h"
#include "extra/ini/writer/writer.h"
#include "extra/serial/endpoints/console.h"
#include "screens/gamescreen.h"
#include "screens/launcherscreen.h"
#include "utils/formatters.hpp"
#include "utils/os.h"
#include "utils/strings.hpp"
#include "window.h"
#include <chrono>
#include <iostream>
#include <map>

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
    int x {};
    int y {};
    uint32_t scaling {};
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
};

Preferences makeDefaultPreferences() {
    Preferences prefs {};
    prefs.scaling = 1;
    prefs.palette = Lcd::DEFAULT_PALETTE;
    prefs.keys.a = SDLK_z;
    prefs.keys.b = SDLK_x;
    prefs.keys.start = SDLK_RETURN;
    prefs.keys.select = SDLK_TAB;
    prefs.keys.left = SDLK_LEFT;
    prefs.keys.up = SDLK_UP;
    prefs.keys.right = SDLK_RIGHT;
    prefs.keys.down = SDLK_DOWN;
    return prefs;
}

void dumpCartridgeInfo(const ICartridge& cartridge) {
    const auto header = CartridgeHeader::parse(cartridge);
    std::cout << "Title             :  " << header.titleAsString() << "\n";
    std::cout << "Cartridge type    :  " << hex(header.cartridge_type) << "     (" << header.cartridgeTypeDescription()
              << ")\n";
    std::cout << "Licensee (new)    :  " << hex(header.new_licensee_code) << "  ("
              << header.newLicenseeCodeDescription() << ")\n";
    std::cout << "Licensee (old)    :  " << hex(header.old_licensee_code) << "     ("
              << header.oldLicenseeCodeDescription() << ")\n";
    std::cout << "ROM Size          :  " << hex(header.rom_size) << "     (" << header.romSizeDescription() << ")\n";
    std::cout << "RAM Size          :  " << hex(header.ram_size) << "     (" << header.ramSizeDescription() << ")\n";
    std::cout << "CGB flag          :  " << hex(header.cgb_flag) << "     (" << header.cgbFlagDescription() << ")\n";
    std::cout << "SGB flag          :  " << hex(header.sgb_flag) << "\n";
    std::cout << "Destination Code  :  " << hex(header.destination_code) << "\n";
    std::cout << "Rom Version Num.  :  " << hex(header.rom_version_number) << "\n";
    std::cout << "Header checksum   :  " << hex(header.header_checksum) << "\n";
}

template <typename T>
std::optional<T> parseInt(const std::string& s) {
    const char* cstr = s.c_str();
    char* endptr {};
    errno = 0;

    T val = std::strtol(cstr, &endptr, 10);

    if (errno || endptr == cstr || *endptr != '\0') {
        return std::nullopt;
    }

    return val;
}

std::optional<uint16_t> parseHexUint16(const std::string& s) {
    const char* cstr = s.c_str();
    char* endptr {};
    errno = 0;

    uint16_t val = std::strtol(cstr, &endptr, 16);

    if (errno || endptr == cstr || *endptr != '\0') {
        return std::nullopt;
    }

    return val;
}

std::optional<SDL_Keycode> parseKeycode(const std::string& s) {
    SDL_Keycode keycode = SDL_GetKeyFromName(s.c_str());
    return keycode != SDLK_UNKNOWN ? std::optional {keycode} : std::nullopt;
}

template <uint8_t Size>
std::optional<std::array<uint16_t, Size>> parseHexArray(const std::string& s) {
    std::vector<std::string> tokens;
    split(s, std::back_inserter(tokens), [](char ch) {
        return ch == ',';
    });

    if (tokens.size() != Size)
        return std::nullopt;

    std::array<uint16_t, Size> ret {};

    for (uint32_t i = 0; i < Size; i++) {
        if (auto u = parseHexUint16(tokens[i]))
            ret[i] = *u;
        else
            return std::nullopt;
    }

    return ret;
}

void ensureFileExists(const std::string& path) {
    if (!file_exists(path)) {
        std::cerr << "ERROR: failed to load '" << path << "'" << std::endl;
        exit(1);
    }
}

std::string getPreferencesPath() {
    char* prefPathCStr = SDL_GetPrefPath("", "DocBoy");
    std::string prefPath = (path(prefPathCStr) / "prefs.ini").string();
    free(prefPathCStr);
    return prefPath;
}

void readPreferences(const std::string& path, Preferences& p) {
    IniReader iniReader;
    iniReader.addCommentPrefix("#");
    iniReader.addProperty("dmg_palette", p.palette, parseHexArray<4>);
    iniReader.addProperty("x", p.x, parseInt<int32_t>);
    iniReader.addProperty("y", p.y, parseInt<int32_t>);
    iniReader.addProperty("scaling", p.scaling, parseInt<uint32_t>);
    iniReader.addProperty("a", p.keys.a, parseKeycode);
    iniReader.addProperty("b", p.keys.b, parseKeycode);
    iniReader.addProperty("start", p.keys.start, parseKeycode);
    iniReader.addProperty("select", p.keys.select, parseKeycode);
    iniReader.addProperty("left", p.keys.left, parseKeycode);
    iniReader.addProperty("up", p.keys.up, parseKeycode);
    iniReader.addProperty("right", p.keys.right, parseKeycode);
    iniReader.addProperty("down", p.keys.down, parseKeycode);

    const auto result = iniReader.parse(path);
    switch (result.outcome) {
    case IniReader::Result::Outcome::Success:
        break;
    case IniReader::Result::Outcome::ErrorReadFailed:
        std::cerr << "ERROR: failed to read '" << path << "'" << std::endl;
        exit(2);
    case IniReader::Result::Outcome::ErrorParseFailed:
        std::cerr << "ERROR: failed to parse  '" << path << "': error at line " << result.lastReadLine << std::endl;
        exit(2);
    }
}

void writePreferences(const std::string& path, const Preferences& p) {
    std::map<std::string, std::string> properties;

    properties.emplace("dmg_palette", join(p.palette, ",", [](uint16_t val) {
                           return hex(val);
                       }));
    properties.emplace("x", std::to_string(p.x));
    properties.emplace("y", std::to_string(p.y));
    properties.emplace("scaling", std::to_string(p.scaling));
    properties.emplace("a", SDL_GetKeyName(p.keys.a));
    properties.emplace("b", SDL_GetKeyName(p.keys.b));
    properties.emplace("start", SDL_GetKeyName(p.keys.start));
    properties.emplace("select", SDL_GetKeyName(p.keys.select));
    properties.emplace("left", SDL_GetKeyName(p.keys.left));
    properties.emplace("up", SDL_GetKeyName(p.keys.up));
    properties.emplace("right", SDL_GetKeyName(p.keys.right));
    properties.emplace("down", SDL_GetKeyName(p.keys.down));

    IniWriter iniWriter;
    if (!iniWriter.write(properties, path)) {
        std::cerr << "WARN: failed to write '" << path << "'" << std::endl;
    }
}
} // namespace

int main(int argc, char* argv[]) {
    // Parse command line arguments
    struct {
        std::string rom;
        IF_BOOTROM(std::string bootRom);
        std::string config {};
        bool serial {};
        uint32_t scaling {};
        bool dumpCartridgeInfo {};
        IF_DEBUGGER(bool debugger {});
    } args;

    Args::Parser argsParser {};
    IF_BOOTROM(argsParser.addArgument(args.bootRom, "boot-rom").help("Boot ROM"));
    argsParser.addArgument(args.rom, "rom").required(false).help("ROM");
    argsParser.addArgument(args.config, "--config", "-c").help("Read configuration file");
#ifdef ENABLE_SERIAL
    argsParser.addArgument(args.serial, "--serial", "-s").help("Display serial console");
#endif
    argsParser.addArgument(args.scaling, "--scaling", "-z").help("Scaling factor");
    argsParser.addArgument(args.dumpCartridgeInfo, "--cartridge-info", "-i").help("Dump cartridge info and quit");
    IF_DEBUGGER(argsParser.addArgument(args.debugger, "--debugger", "-d").help("Attach debugger"));

    // Parse command line arguments
    if (!argsParser.parse(argc, argv, 1))
        return 1;

    // Eventually just dump cartridge info and quit
    if (!args.rom.empty() && args.dumpCartridgeInfo) {
        dumpCartridgeInfo(*CartridgeFactory().create(args.rom));
        return 0;
    }

    // Create default preferences
    Preferences prefs {makeDefaultPreferences()};

    // Eventually load preferences
    std::string prefPath {getPreferencesPath()};
    if (file_exists(prefPath)) {
        readPreferences(prefPath, prefs);
    }

    // Eventually load configuration file (override preferences)
    if (!args.config.empty()) {
        ensureFileExists(args.config);
        readPreferences(args.config, prefs);
    }

    // Args override both preferences and config
    if (args.scaling > 0) {
        prefs.scaling = args.scaling;
    }

    prefs.scaling = std::max(prefs.scaling, 1U);

#ifdef ENABLE_BOOTROM
    ensureFileExists(args.bootRom);
    if (file_size(args.bootRom) != BootRom::Size) {
        std::cerr << "ERROR: invalid boot rom '" << args.bootRom << "'" << std::endl;
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
    auto gb {std::make_unique<GameBoy>(prefs.palette IF_BOOTROM(COMMA BootRomFactory().create(args.bootRom)))};
    Core core {*gb};

    // Create SDL window
    Window window {{prefs.x, prefs.y, prefs.scaling}};

    // Create screens' context
    CoreController coreController {core};
    UiController uiController {window, core};
    NavController navController {window.getScreenStack()};
    MainController mainController {};
#ifdef ENABLE_DEBUGGER
    DebuggerController debuggerController {window, core, coreController};
#endif
    Screen::Context context {
        {coreController, uiController, navController, mainController IF_DEBUGGER(COMMA debuggerController)}, {0xFF}};

    // Setup context accordingly to preferences

    // - Keymap
    coreController.setKeyMapping(prefs.keys.a, Joypad::Key::A);
    coreController.setKeyMapping(prefs.keys.b, Joypad::Key::B);
    coreController.setKeyMapping(prefs.keys.start, Joypad::Key::Start);
    coreController.setKeyMapping(prefs.keys.select, Joypad::Key::Select);
    coreController.setKeyMapping(prefs.keys.left, Joypad::Key::Left);
    coreController.setKeyMapping(prefs.keys.up, Joypad::Key::Up);
    coreController.setKeyMapping(prefs.keys.right, Joypad::Key::Right);
    coreController.setKeyMapping(prefs.keys.down, Joypad::Key::Down);

    // - Scaling
    uiController.setScaling(prefs.scaling);

    // - Palette
    uiController.addPalette({0x84A0, 0x4B40, 0x2AA0, 0x1200}, "Green", 0xFFFF);
    uiController.addPalette({0xFFFF, 0xAD55, 0x52AA, 0x0000}, "Grey", 0xA901);
    const auto* palette = uiController.getPalette(prefs.palette);
    if (!palette) {
        // No know palette exists for this configuration: add a new one
        palette = &uiController.addPalette(prefs.palette, "User");
    }
    uiController.setCurrentPalette(palette->index);

#ifdef ENABLE_SERIAL
    // Eventually attach serial
    std::unique_ptr<SerialConsole> serialConsole;
    std::unique_ptr<SerialLink> serialLink;
    if (args.serial) {
        serialConsole = std::make_unique<SerialConsole>(std::cerr, 16);
        serialLink = std::make_unique<SerialLink>();
        serialLink->plug1.attach(*serialConsole);
        core.attachSerialLink(serialLink->plug2);
    }
#endif

#ifdef ENABLE_DEBUGGER
    // Eventually attach debugger
    if (args.debugger) {
        debuggerController.attachDebugger();
    }
#endif

    if (!args.rom.empty()) {
        // Start with loaded game
        coreController.loadRom(args.rom);
        navController.push(std::make_unique<GameScreen>(context));
    } else {
        // Start with launcher screen instead
        navController.push(std::make_unique<LauncherScreen>(context));
    }

    // Main loop
    SDL_Event e;
    std::chrono::high_resolution_clock::time_point nextFrameTime = std::chrono::high_resolution_clock::now();

    while (!mainController.shouldQuit() IF_DEBUGGER(&&!core.isDebuggerAskingToShutdown())) {
        // Wait until next frame
        while (std::chrono::high_resolution_clock::now() < nextFrameTime)
            ;
        nextFrameTime += mainController.getFrameTime();

        // Handle events
        while (SDL_PollEvent(&e) != 0) {
            switch (e.type) {
            case SDL_EVENT_QUIT:
                mainController.quit();
                break;
            default:
                window.handleEvent(e);
            }
        }

        // Eventually advance emulation by one frame
        if (!coreController.isPaused()) {
            coreController.frame();
        }

        // Render current frame
        window.render();
    }

    // Write RAM state
    if (coreController.isRomLoaded()) {
        coreController.writeSave();
    }

    // Write current preferences
    Window::Geometry geometry = window.getGeometry();
    prefs.x = geometry.x;
    prefs.y = geometry.y;
    prefs.scaling = geometry.scaling;
    prefs.palette = uiController.getCurrentPalette().rgb565.palette;

    const auto& keycodes {coreController.getJoypadMap()};
    prefs.keys.a = keycodes.at(Joypad::Key::A);
    prefs.keys.b = keycodes.at(Joypad::Key::B);
    prefs.keys.start = keycodes.at(Joypad::Key::Start);
    prefs.keys.select = keycodes.at(Joypad::Key::Select);
    prefs.keys.left = keycodes.at(Joypad::Key::Left);
    prefs.keys.up = keycodes.at(Joypad::Key::Up);
    prefs.keys.right = keycodes.at(Joypad::Key::Right);
    prefs.keys.down = keycodes.at(Joypad::Key::Down);

    writePreferences(prefPath, prefs);

#ifdef ENABLE_SERIAL
    if (serialConsole) {
        serialConsole->flush();
    }
#endif

    return 0;
}