#include "argumentum/argparser.h"
#include "docboy/cartridge/factory.h"
#include "docboy/core/core.h"
#include "extra/cartridge/header.h"
#include "extra/serial/endpoints/console.h"
#include "os.h"
#include "utils/formatters.hpp"
#include "utils/io.h"
#include "utils/path.h"
#include "window.h"
#include <SDL.h>
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

static constexpr uint64_t OVERLAY_TEXT_GUID = 1;
static constexpr uint64_t FPS_TEXT_GUID = 2;
static constexpr uint64_t SPEED_TEXT_GUID = 3;

static constexpr std::chrono::nanoseconds DEFAULT_REFRESH_PERIOD {1000000000LU * Specs::Ppu::DOTS_PER_FRAME /
                                                                  Specs::Frequencies::CLOCK};

static const std::map<SDL_Keycode, Joypad::Key> SDL_KEYS_TO_JOYPAD_KEYS = {
    {SDLK_RETURN, Joypad::Key::Start}, {SDLK_TAB, Joypad::Key::Select}, {SDLK_z, Joypad::Key::A},
    {SDLK_x, Joypad::Key::B},          {SDLK_UP, Joypad::Key::Up},      {SDLK_RIGHT, Joypad::Key::Right},
    {SDLK_DOWN, Joypad::Key::Down},    {SDLK_LEFT, Joypad::Key::Left},
};

static void dump_cartridge_info(const ICartridge& cartridge) {
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

int main(int argc, char* argv[]) {
    struct {
        std::string rom;
        IF_BOOTROM(std::string bootRom);
        bool serial {};
        float scaling {};
        bool dumpCartridgeInfo {};
        IF_DEBUGGER(bool debugger {});
    } args;

    argumentum::argument_parser parser {};
    auto params = parser.params();
    IF_BOOTROM(params.add_parameter(args.bootRom, "boot-rom").help("Boot ROM"));
    params.add_parameter(args.rom, "rom").help("ROM");
    params.add_parameter(args.serial, "--serial", "-s").help("Display serial console");
    params.add_parameter(args.scaling, "--scaling", "-z").nargs(1).default_value(1).help("Scaling factor");
    params.add_parameter(args.dumpCartridgeInfo, "--cartridge-info", "-i").help("Dump cartridge info and quit");
    IF_DEBUGGER(params.add_parameter(args.debugger, "--debugger", "-d").help("Attach debugger"));

    if (!parser.parse_args(argc, argv, 1))
        return 1;

    const auto ensureExists = [](const std::string& path) {
        if (!file_exists(path)) {
            std::cerr << "ERROR: failed to load '" << path << "'" << std::endl;
            exit(1);
        }
    };

    IF_BOOTROM(ensureExists(args.bootRom));
    ensureExists(args.rom);

    std::unique_ptr<GameBoy> gb {std::make_unique<GameBoy>(IF_BOOTROM(BootRomFactory().create(args.bootRom)))};
    Core core {*gb};

    path romPath {args.rom};

    std::unique_ptr<ICartridge> cartridge {CartridgeFactory().create(romPath.string())};

    if (args.dumpCartridgeInfo) {
        dump_cartridge_info(*cartridge);
        return 0;
    }

    core.loadRom(std::move(cartridge));

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

    Window window {gb->lcd.getPixels(), 100, 100, args.scaling};

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
                       OVERLAY_TEXT_GUID);
    };

    const auto drawFPS = [&window, &fps]() {
        std::string fpsString = std::to_string(fps);
        window.addText(fpsString,
                       static_cast<int>(Window::WINDOW_WIDTH - 4 - fpsString.size() * Window::TEXT_LETTER_WIDTH), 4,
                       0xFFFFFFFF, Window::TEXT_DURATION_PERSISTENT, FPS_TEXT_GUID);
    };

    const auto hideFPS = [&window]() {
        window.removeText(FPS_TEXT_GUID);
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
                           0xFFFFFFFF, Window::TEXT_DURATION_PERSISTENT, SPEED_TEXT_GUID);
        else
            window.removeText(SPEED_TEXT_GUID);
    };

    const auto screenshotPNG = [&window](const std::string& path) {
        return window.screenshot(path);
    };

    const auto dumpFramebuffer = [&gb](const std::string& path) {
        bool ok;
        write_file(path, gb->lcd.getPixels(), Specs::Display::WIDTH * Specs::Display::HEIGHT * sizeof(uint16_t), &ok);
        return ok;
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

    const auto writeState = [&core](const std::string& path) {
        std::vector<uint8_t> data(core.getStateSaveSize());
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

        if (data.size() != core.getStateSaveSize())
            return false;

        core.loadState(data.data());

        return ok;
    };

    const std::string savePath = romPath.replace_extension("sav").string();
    const std::string statePath = romPath.replace_extension("state").string();

    // eventually load ram state
    readSave(savePath);

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
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_KEYDOWN: {
                switch (e.key.keysym.sym) {
                case SDLK_F1:
                    if (writeState(statePath))
                        drawOverlay("State saved");
                    break;
                case SDLK_F2:
                    if (readState(statePath))
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
            case SDL_KEYUP: {
                handleInput(e.key.keysym.sym, Joypad::KeyState::Released);
                break;
            }
            }
        }

        // update emulator until next frame
        core.frame();

        // render frame
        window.render();

        // update FPS estimation
        updateFPS();
    }

    // write ram state
    writeSave(savePath);

#ifdef ENABLE_SERIAL
    if (serialConsole) {
        serialConsole->flush();
    }
#endif

    return 0;
}