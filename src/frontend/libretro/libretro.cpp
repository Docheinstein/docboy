#include "libretro.h"

#include "docboy/core/core.h"

#include <cstdarg>
#include <cstring>

namespace {
constexpr unsigned TWO_PLAYERS_MODE_ID = 0x101;

constexpr unsigned PLAYER_1_RETRO_MEMORY_SAVE_RAM = 0x100 | RETRO_MEMORY_SAVE_RAM;
constexpr unsigned PLAYER_2_RETRO_MEMORY_SAVE_RAM = 0x200 | RETRO_MEMORY_SAVE_RAM;

constexpr unsigned PLAYER_1_RETRO_MEMORY_SAVE_RTC = 0x100 | RETRO_MEMORY_RTC;
constexpr unsigned PLAYER_2_RETRO_MEMORY_SAVE_RTC = 0x200 | RETRO_MEMORY_RTC;

constexpr uint32_t AUDIO_SAMPLE_RATE = 32768;

constexpr uint32_t CYCLES_PER_BURST = 4;

retro_log_printf_t retro_log;

retro_environment_t environ_cb;
retro_video_refresh_t video_cb;
retro_audio_sample_batch_t audio_batch_cb;
retro_input_poll_t input_poll_cb;
retro_input_state_t input_state_cb;
retro_rumble_interface rumble {};

uint8_t num_players {1};

GameBoy gameboy[2] {};

Core core[2] {Core {gameboy[0]}, Core {gameboy[1]}};

uint16_t* framebuffer_two_players {};

struct {
    int16_t data[32768] {};
    uint32_t index {};
} audiobuffer;

void retro_fallback_log(enum retro_log_level level, const char* fmt, ...) {
    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
}

void audio_sample_cb(const Apu::AudioSample sample) {
    if (audiobuffer.index < array_size(audiobuffer.data) - 1) {
        audiobuffer.data[audiobuffer.index++] = sample.left;
        audiobuffer.data[audiobuffer.index++] = sample.right;
    }
}

void rumble_cb(uint8_t player, bool enabled) {
    if (rumble.set_rumble_state) {
        rumble.set_rumble_state(player, RETRO_RUMBLE_STRONG, enabled ? 0xFFFF : 0x0000);
    }
}
} // namespace

void retro_init(void) {
    // TODO: give the user the option to select the core source of audio?
    core[0].set_audio_sample_rate(AUDIO_SAMPLE_RATE);
    core[0].set_audio_sample_callback(audio_sample_cb);
}

void retro_deinit(void) {
    if (num_players == 2) {
        free(framebuffer_two_players);
        framebuffer_two_players = nullptr;
    }
}

unsigned retro_api_version(void) {
    return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device) {
}

void retro_get_system_info(struct retro_system_info* info) {
    memset(info, 0, sizeof(*info));
    info->library_name = "DocBoy";
    info->library_version = "0.1";
    info->need_fullpath = true;
    info->valid_extensions = "gb|dmg|gbc|cgb|sgb";
}

void retro_get_system_av_info(struct retro_system_av_info* info) {
    info->geometry.base_width = Specs::Display::WIDTH * num_players;
    info->geometry.base_height = Specs::Display::HEIGHT;
    info->geometry.max_width = Specs::Display::WIDTH * num_players;
    info->geometry.max_height = Specs::Display::HEIGHT;

    info->geometry.aspect_ratio = 0.0f;

    info->timing.fps = Specs::FPS;
    info->timing.sample_rate = AUDIO_SAMPLE_RATE;
}

void retro_set_environment(retro_environment_t cb) {
    environ_cb = cb;

    struct retro_log_callback logging {};

    if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging)) {
        retro_log = logging.log;
    } else {
        retro_log = retro_fallback_log;
    }

    static const struct retro_subsystem_memory_info rom1_memory_info[] = {
        {"srm", PLAYER_1_RETRO_MEMORY_SAVE_RAM},
    };

    static const struct retro_subsystem_memory_info rom2_memory_info[] = {
        {"srm", PLAYER_2_RETRO_MEMORY_SAVE_RAM},
    };

    static const struct retro_subsystem_rom_info roms_info[] = {
        {"GameBoy #1", "gb|dmg|gbc|cgb|sgb", true, false, true, rom1_memory_info, 1},
        {"GameBoy #2", "gb|dmg|gbc|cgb|sgb", true, false, true, rom2_memory_info, 1},
    };

    static const struct retro_subsystem_info subsystems[] = {
        {"2 Game Boys Serial Link", "gb2p", roms_info, 2, TWO_PLAYERS_MODE_ID},
        {nullptr},
    };

    environ_cb(RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO, (void*)subsystems);
}

void retro_set_audio_sample(retro_audio_sample_t cb) {
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) {
    audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb) {
    input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb) {
    input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb) {
    video_cb = cb;
}

void retro_reset() {
    for (uint8_t i = 0; i < num_players; i++) {
        core[i].reset();
    }
}

void retro_run(void) {
    // Handle input
    static constexpr uint16_t RETRO_KEYS[] = {
        RETRO_DEVICE_ID_JOYPAD_LEFT,  RETRO_DEVICE_ID_JOYPAD_UP,    RETRO_DEVICE_ID_JOYPAD_DOWN,
        RETRO_DEVICE_ID_JOYPAD_RIGHT, RETRO_DEVICE_ID_JOYPAD_START, RETRO_DEVICE_ID_JOYPAD_SELECT,
        RETRO_DEVICE_ID_JOYPAD_B,     RETRO_DEVICE_ID_JOYPAD_A,
    };

    static constexpr Joypad::Key RETRO_KEYS_TO_GAMEBOY_KEYS[] = {
        Joypad::Key::Left,  Joypad::Key::Up,     Joypad::Key::Down, Joypad::Key::Right,
        Joypad::Key::Start, Joypad::Key::Select, Joypad::Key::B,    Joypad::Key::A,
    };

    input_poll_cb();

    for (uint8_t player = 0; player < num_players; player++) {
        for (uint32_t key_index = 0; key_index < array_size(RETRO_KEYS); key_index++) {
            const uint16_t retro_key = RETRO_KEYS[key_index];
            const bool is_key_pressed = input_state_cb(player, RETRO_DEVICE_JOYPAD, 0, retro_key);
            core[player].set_key(RETRO_KEYS_TO_GAMEBOY_KEYS[key_index],
                                 is_key_pressed ? Joypad::KeyState::Pressed : Joypad::KeyState::Released);
        }
    }

    // Emulate until next frame
    if (num_players == 2) {
        bool frame_done[2] {};

        while (!frame_done[0] || !frame_done[1]) {
            // Advance both cores at the same time by some cycles.
            frame_done[0] |= core[0].run_for_cycles(CYCLES_PER_BURST);
            frame_done[1] |= core[1].run_for_cycles(CYCLES_PER_BURST);
        }
    } else {
        core[0].frame();
    }

    // Draw framebuffer
    const uint16_t* framebuffer;

    if (num_players == 2) {
        // Merge the framebuffers into one
        const uint16_t* framebuffer0 = gameboy[0].lcd.get_pixels();
        const uint16_t* framebuffer1 = gameboy[1].lcd.get_pixels();

        for (uint8_t row = 0; row < Specs::Display::HEIGHT; ++row) {
            memcpy(framebuffer_two_players + Specs::Display::WIDTH * 2 * row,
                   framebuffer0 + Specs::Display::WIDTH * row, Specs::Display::WIDTH * sizeof(uint16_t));
            memcpy(framebuffer_two_players + Specs::Display::WIDTH * 2 * row + Specs::Display::WIDTH,
                   framebuffer1 + Specs::Display::WIDTH * row, Specs::Display::WIDTH * sizeof(uint16_t));
        }

        framebuffer = framebuffer_two_players;
    } else {
        framebuffer = gameboy[0].lcd.get_pixels();
    }

    video_cb(framebuffer, Specs::Display::WIDTH * num_players, Specs::Display::HEIGHT,
             Specs::Display::WIDTH * sizeof(uint16_t) * num_players);

    audio_batch_cb(audiobuffer.data, audiobuffer.index / 2);
    audiobuffer.index = 0;
}

bool retro_load_game(const struct retro_game_info* info) {
    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
    if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt)) {
        retro_log(RETRO_LOG_ERROR, "RGB565 not supported.\n");
        return false;
    }

    bool rumble_supported = environ_cb(RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE, &rumble);
    if (rumble_supported) {
        retro_log(RETRO_LOG_INFO, "Rumble supported.\n");
    } else {
        retro_log(RETRO_LOG_INFO, "Rumble not supported.\n");
    }

    static const struct retro_controller_description controllers[] = {
        {"Nintendo Gameboy", RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0)},
    };

    static const struct retro_controller_info controller_info[] = {
        {controllers, 1},
        {nullptr, 0},
    };

    environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)controller_info);

    static struct retro_input_descriptor input_descriptor[] = {
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "Left"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Up"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Down"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "B"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "A"},
        {0},
    };

    environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, input_descriptor);

    retro_log(RETRO_LOG_INFO, "Loading rom %s\n", info->path);
    core[0].load_rom(info->path);

    if (rumble_supported) {
        core[0].set_rumble_callback([](const bool enabled) {
            rumble_cb(0, enabled);
        });
    }

    return true;
}

void retro_unload_game(void) {
}

unsigned retro_get_region(void) {
    return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info* info, size_t num) {
    if (type != TWO_PLAYERS_MODE_ID) {
        retro_log(RETRO_LOG_WARN, "Unsupported special game mode %u\n", type);
        return false;
    }

    if (num > 2) {
        retro_log(RETRO_LOG_WARN, "Unsupported number of roms %u\n", num);
        return false;
    }

    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
    if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt)) {
        retro_log(RETRO_LOG_ERROR, "RGB565 not supported.\n");
        return false;
    }

    bool rumble_supported = environ_cb(RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE, &rumble);
    if (rumble_supported) {
        retro_log(RETRO_LOG_INFO, "Rumble supported.\n");
    } else {
        retro_log(RETRO_LOG_INFO, "Rumble not supported.\n");
    }

    static const struct retro_controller_description controllers[] = {
        {"Nintendo Gameboy", RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0)},
    };

    static const struct retro_controller_info controller_info[] = {
        {controllers, 1},
        {controllers, 1},
        {nullptr, 0},
    };

    environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)controller_info);

    static struct retro_input_descriptor input_descriptor[] = {
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "Left"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Up"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Down"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "B"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "A"},
        {1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "Left"},
        {1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Up"},
        {1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Down"},
        {1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right"},
        {1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start"},
        {1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select"},
        {1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "B"},
        {1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "A"},
        {0},
    };

    environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, input_descriptor);

    num_players = num;

    if (!framebuffer_two_players) {
        // Allocate the framebuffer for the two-players screen.
        framebuffer_two_players =
            (uint16_t*)malloc(Specs::Display::WIDTH * Specs::Display::HEIGHT * sizeof(uint16_t) * num_players);
    }

    for (uint8_t i = 0; i < num_players; ++i) {
        retro_log(RETRO_LOG_INFO, "Loading rom for GameBoy %u: %s\n", i, info[i].path);

        // Load ROM.
        core[i].load_rom(info[i].path);

        // Attach the cores each other through the serial link.
        core[i].attach_serial_link(gameboy[1 - i].serial);

        if (rumble_supported) {
            core[i].set_rumble_callback([i](const bool enabled) {
                rumble_cb(i, enabled);
            });
        }
    }

    return true;
}

size_t retro_serialize_size(void) {
    return core[0].get_state_size() * num_players;
}

bool retro_serialize(void* data, size_t size) {
    for (uint8_t i = 0; i < num_players; ++i) {
        core[i].save_state((uint8_t*)data + i * core[0].get_state_size());
    }

    return true;
}

bool retro_unserialize(const void* data, size_t size) {
    for (uint8_t i = 0; i < num_players; ++i) {
        core[i].load_state((uint8_t*)data + i * core[0].get_state_size());
    }

    return true;
}

void* retro_get_memory_data(unsigned id) {
    if (num_players == 2) {
        // Two players mode
        switch (id) {
        case PLAYER_1_RETRO_MEMORY_SAVE_RAM:
            return gameboy[0].cartridge_slot.cartridge->get_ram_save_data();
        case PLAYER_1_RETRO_MEMORY_SAVE_RTC:
            return gameboy[0].cartridge_slot.cartridge->get_rtc_save_data();
        case PLAYER_2_RETRO_MEMORY_SAVE_RAM:
            return gameboy[1].cartridge_slot.cartridge->get_ram_save_data();
        case PLAYER_2_RETRO_MEMORY_SAVE_RTC:
            return gameboy[1].cartridge_slot.cartridge->get_rtc_save_data();
        default:
            return nullptr;
        }
    }

    // One player mode
    switch (id) {
    case RETRO_MEMORY_SAVE_RAM:
        return gameboy[0].cartridge_slot.cartridge->get_ram_save_data();
    case RETRO_MEMORY_RTC:
        return gameboy[0].cartridge_slot.cartridge->get_rtc_save_data();
    default:
        return nullptr;
    }
}

size_t retro_get_memory_size(unsigned id) {
    if (num_players == 2) {
        // Two players mode
        switch (id) {
        case PLAYER_1_RETRO_MEMORY_SAVE_RAM:
            return gameboy[0].cartridge_slot.cartridge->get_ram_save_size();
        case PLAYER_1_RETRO_MEMORY_SAVE_RTC:
            return gameboy[0].cartridge_slot.cartridge->get_rtc_save_size();
        case PLAYER_2_RETRO_MEMORY_SAVE_RAM:
            return gameboy[1].cartridge_slot.cartridge->get_ram_save_size();
        case PLAYER_2_RETRO_MEMORY_SAVE_RTC:
            return gameboy[1].cartridge_slot.cartridge->get_rtc_save_size();
        default:
            return 0;
        }
    }

    // One player mode
    switch (id) {
    case RETRO_MEMORY_SAVE_RAM:
        return gameboy[0].cartridge_slot.cartridge->get_ram_save_size();
    case RETRO_MEMORY_RTC:
        return gameboy[0].cartridge_slot.cartridge->get_rtc_save_size();
    default:
        return 0;
    }
}

void retro_cheat_reset(void) {
}

void retro_cheat_set(unsigned index, bool enabled, const char* code) {
}
