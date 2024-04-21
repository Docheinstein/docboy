#include "args/args.h"
#include "docboy/cartridge/factory.h"
#include "docboy/core/core.h"
#include "extra/cartridge/header.h"
#include "extra/ini/reader/reader.h"
#include "extra/ini/writer/writer.h"
#include "extra/serial/endpoints/console.h"
#include "utils/formatters.hpp"
#include "utils/io.h"
#include "utils/mathematics.h"
#include "utils/os.h"
#include "utils/path.h"
#include "utils/strings.hpp"
#include "window.h"
#include <SDL3/SDL.h>
#include <chrono>
#include <iostream>
#include <map>

#ifdef ENABLE_BOOTROM
#include "docboy/bootrom/factory.h"
#endif

#ifdef ENABLE_DEBUGGER
#include "docboy/debugger/backend.h"
#include "docboy/debugger/frontend.h"
#include "docboy/debugger/memsniffer.h"
#endif

namespace {
namespace TextGuids {
    constexpr uint64_t DROP_ROM = 1;
    constexpr uint64_t OVERLAY = 2;
    constexpr uint64_t FPS = 3;
    constexpr uint64_t SPEED = 4;
} // namespace TextGuids

constexpr std::chrono::nanoseconds DEFAULT_REFRESH_PERIOD {1000000000LU * Specs::Ppu::DOTS_PER_FRAME /
                                                           Specs::Frequencies::CLOCK};

const std::map<SDL_Keycode, Joypad::Key> SDL_KEYS_TO_JOYPAD_KEYS = {
    {SDLK_RETURN, Joypad::Key::Start}, {SDLK_TAB, Joypad::Key::Select}, {SDLK_z, Joypad::Key::A},
    {SDLK_x, Joypad::Key::B},          {SDLK_UP, Joypad::Key::Up},      {SDLK_RIGHT, Joypad::Key::Right},
    {SDLK_DOWN, Joypad::Key::Down},    {SDLK_LEFT, Joypad::Key::Left},
};

void dump_cartridge_info(const ICartridge& cartridge) {
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
std::optional<T> string_to_integer(const std::string& s) {
    const char* cstr = s.c_str();
    char* endptr {};
    errno = 0;

    T val = std::strtol(cstr, &endptr, 10);

    if (errno || endptr == cstr || *endptr != '\0') {
        return std::nullopt;
    }

    return val;
}

template <typename T>
std::optional<T> string_to_float(const std::string& s) {
    const char* cstr = s.c_str();
    char* endptr {};
    errno = 0;

    T val = std::strtof(cstr, &endptr);

    if (errno || endptr == cstr || *endptr != '\0') {
        return std::nullopt;
    }

    return val;
}

std::optional<uint16_t> hex_string_to_uint16(const std::string& s) {
    const char* cstr = s.c_str();
    char* endptr {};
    errno = 0;

    uint16_t val = std::strtol(cstr, &endptr, 16);

    if (errno || endptr == cstr || *endptr != '\0') {
        return std::nullopt;
    }

    return val;
}

template <uint8_t Size>
std::optional<std::array<uint16_t, Size>> parse_hex_string_array(const std::string& s) {
    std::vector<std::string> tokens;
    split(s, std::back_inserter(tokens), [](char ch) {
        return ch == ',';
    });

    if (tokens.size() != Size)
        return std::nullopt;

    std::array<uint16_t, Size> ret {};

    for (uint32_t i = 0; i < Size; i++) {
        if (auto u = hex_string_to_uint16(tokens[i]))
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
} // namespace

int main(int argc, char* argv[]) {
    // Parse command line arguments
    struct {
        std::string rom;
        IF_BOOTROM(std::string bootRom);
        std::string config {};
        bool serial {};
        float scaling {1.0};
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

    if (!argsParser.parse(argc, argv, 1))
        return 1;

    if (!args.rom.empty() && args.dumpCartridgeInfo) {
        // Just dump cartridge info and quit.
        dump_cartridge_info(*CartridgeFactory().create(args.rom));
        return 0;
    }

    struct Preferences {
        Lcd::Palette palette {Lcd::DEFAULT_PALETTE};
        int32_t x {};
        int32_t y {};
        float scaling {};
    } prefs {};

    prefs.scaling = args.scaling;

    const auto readPreferences = [](const std::string& path, Preferences& p) {
        IniReader iniReader;
        iniReader.addCommentPrefix("#");
        iniReader.addProperty("dmg_palette", p.palette, parse_hex_string_array<4>);
        iniReader.addProperty("x", p.x, string_to_integer<int32_t>);
        iniReader.addProperty("y", p.y, string_to_integer<int32_t>);
        iniReader.addProperty("scaling", p.scaling, string_to_float<float>);

        if (const auto result = iniReader.parse(path); !result) {
            switch (result.outcome) {
            case IniReader::Result::Outcome::ErrorReadFailed:
                std::cerr << "ERROR: failed to read '" << path << "'" << std::endl;
                break;
            case IniReader::Result::Outcome::ErrorParseFailed:
                std::cerr << "ERROR: failed to parse  '" << path << "': error at line " << result.lastReadLine
                          << std::endl;
                break;
            default:;
            }
            exit(2);
        }
    };

    const auto writePreferences = [](const std::string& path, const Preferences& p) {
        std::map<std::string, std::string> properties;

        properties.emplace("dmg_palette", join(p.palette, ",", [](uint16_t val) {
                               return hex(val);
                           }));
        properties.emplace("x", std::to_string(p.x));
        properties.emplace("y", std::to_string(p.y));
        properties.emplace("scaling", std::to_string(p.scaling));

        IniWriter iniWriter;
        if (!iniWriter.write(properties, path)) {
            std::cerr << "WARN: failed to write '" << path << "'" << std::endl;
        }
    };

    // Eventually load preferences
    char* prefPathCStr = SDL_GetPrefPath("", "DocBoy");
    std::string prefPath = (path(prefPathCStr) / "prefs.ini").string();
    free(prefPathCStr);

    if (file_exists(prefPath)) {
        readPreferences(prefPath, prefs);
    }

    // Eventually load configuration file (override preferences)
    if (!args.config.empty()) {
        ensureFileExists(args.config);
        readPreferences(args.config, prefs);
    }

    IF_BOOTROM(ensureFileExists(args.bootRom));

    auto gb {std::make_unique<GameBoy>(prefs.palette IF_BOOTROM(COMMA BootRomFactory().create(args.bootRom)))};
    Core core {*gb};

    Window window {gb->lcd.getPixels(), {prefs.x, prefs.y, prefs.scaling}};

    path romPath {};

    const auto getSavePath = [&romPath]() {
        return romPath.replace_extension("sav").string();
    };

    const auto getStatePath = [&romPath]() {
        return romPath.replace_extension("state").string();
    };

    const auto writeSave = [&core](const std::string& path) {
        if (!core.canSaveRam())
            return false;

        std::vector<uint8_t> data(core.getRamSaveSize());
        core.saveRam(data.data());

        bool ok;
        write_file(path, data.data(), data.size(), &ok);

        return ok;
    };

    const auto readSave = [&core](const std::string& path) {
        if (!core.canSaveRam())
            return false;

        bool ok;
        std::vector<uint8_t> data = read_file(path, &ok);
        if (!ok)
            return false;

        core.loadRam(data.data());
        return true;
    };

    bool isRomLoaded {};
    const auto loadRom = [&](const std::string& p) {
        if (isRomLoaded) {
            // Eventually save current RAM state
            writeSave(getSavePath());
        }

        ensureFileExists(p);
        romPath = path(p);
        isRomLoaded = true;

        // Actually load ROM
        core.loadRom(p);

        // Eventually load new RAM state
        readSave(getSavePath());

        // TODO: reset debugger
    };

    if (!args.rom.empty()) {
        loadRom(args.rom);
    }

#ifdef ENABLE_SERIAL
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
    struct {
        std::unique_ptr<DebuggerBackend> backend;
        std::unique_ptr<DebuggerFrontend> frontend;
    } debugger;

    const auto isDebuggerAttached = [&debugger]() {
        return debugger.backend != nullptr;
    };

    const auto attachDebugger = [&core, &gb, &debugger, &window, &isDebuggerAttached](bool proceed = false) {
        if (isDebuggerAttached())
            return false;

        debugger.backend = std::make_unique<DebuggerBackend>(core);
        debugger.frontend = std::make_unique<DebuggerFrontend>(*debugger.backend);
        core.attachDebugger(*debugger.backend);
        debugger.backend->attachFrontend(*debugger.frontend);
        DebuggerMemorySniffer::setObserver(&*debugger.backend);

        debugger.frontend->setOnPullingCommandCallback([&gb, &window]() {
            // Cache the next pixel color
            Lcd::PixelRgb565& nextPixel = gb->lcd.getPixels()[gb->lcd.getCursor()];
            const Lcd::PixelRgb565 nextPixelColor = nextPixel;

            // Mark the current dot as a white pixel (useful for debug PPU)
            nextPixel = 0xFFFF;

            // Render framebuffer
            window.render();

            // Restore original color
            nextPixel = nextPixelColor;
        });
        debugger.frontend->setOnCommandPulledCallback([&window, &gb](const std::string& cmd) {
            if (cmd == "clear") {
                memset(gb->lcd.getPixels(), 0, Lcd::PIXEL_BUFFER_SIZE);
                window.clear();
                return true;
            }
            return false;
        });

        if (proceed)
            debugger.backend->proceed();

        return true;
    };

    const auto detachDebugger = [&core, &debugger, &isDebuggerAttached]() {
        if (!isDebuggerAttached())
            return false;

        debugger.backend = nullptr;
        debugger.frontend = nullptr;
        DebuggerMemorySniffer::setObserver(nullptr);
        core.detachDebugger();

        return true;
    };

    if (args.debugger) {
        attachDebugger();
    }
#endif

    SDL_Event e;
    bool quit {false};
    bool showFPS {false};
    uint32_t fps {0};
    int64_t speedLevel {0};

    std::chrono::high_resolution_clock::time_point lastFpsSampling = std::chrono::high_resolution_clock::now();

    const auto drawOverlay = [&](const std::string& str) {
        window.addText(str, 4, Window::WINDOW_HEIGHT - Window::TEXT_LETTER_HEIGHT - 4, 0xFFFFFFFF, 2000,
                       TextGuids::OVERLAY);
    };

    const auto drawFPS = [&window, &fps]() {
        std::string fpsString = std::to_string(fps);
        window.addText(fpsString,
                       static_cast<int>(Window::WINDOW_WIDTH - 4 - fpsString.size() * Window::TEXT_LETTER_WIDTH), 4,
                       0xFFFFFFFF, Window::TEXT_DURATION_PERSISTENT, TextGuids::FPS);
    };

    const auto hideFPS = [&window]() {
        window.removeText(TextGuids::FPS);
    };

    const auto handleInput = [&core](SDL_Keycode key, Joypad::KeyState keyState) {
        if (const auto mapping = SDL_KEYS_TO_JOYPAD_KEYS.find(key); mapping != SDL_KEYS_TO_JOYPAD_KEYS.end()) {
            core.setKey(mapping->second, keyState);
        }
    };

    const auto toggleFPS = [&]() {
        showFPS = !showFPS;
        if (showFPS) {
            lastFpsSampling = std::chrono::high_resolution_clock::now();
            fps = 0;
        } else {
            hideFPS();
        }
    };

    const auto updateFPS = [&]() {
        fps++;
        if (showFPS) {
            const auto now = std::chrono::high_resolution_clock::now();
            if (now - lastFpsSampling > std::chrono::nanoseconds(1000000000)) {
                drawFPS();
                lastFpsSampling = now;
                fps = 0;
            }
        }
    };

    const auto drawSpeed = [&]() {
        if (speedLevel != 0)
            window.addText((speedLevel > 0 ? "x" : "/") + std::to_string((uint32_t)(pow2(abs(speedLevel)))), 4, 4,
                           0xFFFFFFFF, Window::TEXT_DURATION_PERSISTENT, TextGuids::SPEED);
        else
            window.removeText(TextGuids::SPEED);
    };

    const auto screenshotPNG = [&window](const std::string& path) {
        return window.screenshot(path);
    };

    const auto dumpFramebuffer = [&gb](const std::string& path) {
        bool ok;
        write_file(path, gb->lcd.getPixels(), Specs::Display::WIDTH * Specs::Display::HEIGHT * sizeof(uint16_t), &ok);
        return ok;
    };

    const auto writeState = [&core](const std::string& path) {
        std::vector<uint8_t> data(core.getStateSize());
        core.saveState(data.data());

        bool ok;
        write_file(path, data.data(), data.size(), &ok);

        return ok;
    };

    const auto readState = [&core](const std::string& path) {
        bool ok;
        std::vector<uint8_t> data = read_file(path, &ok);
        if (!ok)
            return false;

        if (data.size() != core.getStateSize())
            return false;

        core.loadState(data.data());

        return ok;
    };

    if (!isRomLoaded)
        window.addText("Drop a GB ROM", 27, 66, 0xFFFFFFFF, Window::TEXT_DURATION_PERSISTENT, TextGuids::DROP_ROM);

    std::chrono::high_resolution_clock::time_point nextFrameTime = std::chrono::high_resolution_clock::now();

    while (!quit IF_DEBUGGER(&&!core.isDebuggerAskingToShutdown())) {
        // wait until next frame
        std::chrono::high_resolution_clock::time_point now;
        do {
            now = std::chrono::high_resolution_clock::now();
        } while (now < nextFrameTime);
        nextFrameTime += std::chrono::nanoseconds {(uint64_t)(DEFAULT_REFRESH_PERIOD.count() / pow2(speedLevel))};

        // handle input
        while (SDL_PollEvent(&e) != 0) {
            switch (e.type) {
            case SDL_EVENT_QUIT:
                quit = true;
                break;
            case SDL_EVENT_KEY_DOWN: {
                switch (e.key.keysym.sym) {
                case SDLK_F1:
                    if (writeState(getStatePath()))
                        drawOverlay("State saved");
                    break;
                case SDLK_F2:
                    if (readState(getStatePath()))
                        drawOverlay("State loaded");
                    break;
                case SDLK_F11:
                    if (dumpFramebuffer((temp_directory_path() / romPath.replace_extension("dat").filename()).string()))
                        drawOverlay("Framebuffer saved");
                    break;
                case SDLK_F12:
                    if (screenshotPNG((temp_directory_path() / romPath.replace_extension("png").filename()).string()))
                        drawOverlay("Screenshot saved");
                    break;
                case SDLK_f:
                    toggleFPS();
                    break;
                case SDLK_q:
                    --speedLevel;
                    drawSpeed();
                    break;
                case SDLK_w:
                    ++speedLevel;
                    drawSpeed();
                    break;
#ifdef ENABLE_DEBUGGER
                case SDLK_d:
                    if (isDebuggerAttached()) {
                        if (detachDebugger())
                            drawOverlay("Debugger detached");
                    } else {
                        if (attachDebugger(true))
                            drawOverlay("Debugger attached");
                    }
                    break;
#endif
                default:
                    handleInput(e.key.keysym.sym, Joypad::KeyState::Pressed);
                    break;
                }
                break;
            }
            case SDL_EVENT_KEY_UP: {
                handleInput(e.key.keysym.sym, Joypad::KeyState::Released);
                break;
            }
            case SDL_EVENT_DROP_FILE: {
                // File dropped: load ROM.
                loadRom(e.drop.data);
                window.removeText(TextGuids::DROP_ROM);
                break;
            }
            }
        }

        // Update emulator until next frame
        if (isRomLoaded)
            core.frame();

        // Render frame
        window.render();

        // Update FPS estimation
        updateFPS();
    }

    // Write RAM state
    if (isRomLoaded) {
        writeSave(getSavePath());
    }

    // Write preferences
    Window::Geometry geometry = window.getGeometry();
    prefs.x = geometry.x;
    prefs.y = geometry.y;
    prefs.scaling = geometry.scaling;

    writePreferences(prefPath, prefs);

#ifdef ENABLE_SERIAL
    if (serialConsole) {
        serialConsole->flush();
    }
#endif

    return 0;
}