#include "libretro.h"
#include "docboy/core/core.h"
#include <cstdarg>
#include <cstring>

namespace {
retro_log_printf_t retro_log;

retro_environment_t environ_cb;
retro_video_refresh_t video_cb;
retro_audio_sample_batch_t audio_batch_cb;
retro_input_poll_t input_poll_cb;
retro_input_state_t input_state_cb;

GameBoy gameboy {};
Core core {gameboy};
const uint16_t* framebuffer {gameboy.lcd.get_pixels()};

int16_t audiobuffer[4096] {};

void retro_fallback_log(enum retro_log_level level, const char* fmt, ...) {
    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
}
} // namespace

void retro_init(void) {
}

void retro_deinit(void) {
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
    info->geometry.base_width = Specs::Display::WIDTH;
    info->geometry.base_height = Specs::Display::HEIGHT;
    info->geometry.max_width = Specs::Display::WIDTH;
    info->geometry.max_height = Specs::Display::HEIGHT;
    info->geometry.aspect_ratio = 0.0f;
}

void retro_set_environment(retro_environment_t cb) {
    environ_cb = cb;

    struct retro_log_callback logging {};

    if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging)) {
        retro_log = logging.log;
    } else {
        retro_log = retro_fallback_log;
    }

    static const struct retro_controller_description controllers[] = {
        {"Nintendo Gameboy", RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0)},
    };

    static const struct retro_controller_info ports[] = {
        {controllers, 1},
        {nullptr, 0},
    };

    cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);
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

    for (uint32_t key_index = 0; key_index < array_size(RETRO_KEYS); key_index++) {
        const uint16_t retro_key = RETRO_KEYS[key_index];
        const bool is_key_pressed = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, retro_key);
        core.set_key(RETRO_KEYS_TO_GAMEBOY_KEYS[key_index],
                     is_key_pressed ? Joypad::KeyState::Pressed : Joypad::KeyState::Released);
    }

    // Emulate until next frame
    core.frame();

    // Draw framebuffer
    video_cb(framebuffer, Specs::Display::WIDTH, Specs::Display::HEIGHT, Specs::Display::WIDTH * sizeof(uint16_t));

    // TODO: audio is needed to limit emulation at 60fps
    audio_batch_cb(audiobuffer, 1476 / 2);
}

bool retro_load_game(const struct retro_game_info* info) {
    struct retro_input_descriptor desc[] = {
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

    environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
    if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt)) {
        retro_log(RETRO_LOG_ERROR, "RGB565 is not supported.\n");
        return false;
    }

    retro_log(RETRO_LOG_INFO, "Loading: %s", info->path);
    core.load_rom(info->path);
    retro_log(RETRO_LOG_INFO, "Loaded: %s", info->path);
    return true;
}

void retro_unload_game(void) {
}

unsigned retro_get_region(void) {
    return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info* info, size_t num) {
    return false;
}

size_t retro_serialize_size(void) {
    return core.get_state_size();
}

bool retro_serialize(void* data, size_t size) {
    core.save_state(data);
    return true;
}

bool retro_unserialize(const void* data, size_t size) {
    core.load_state(data);
    return true;
}

void* retro_get_memory_data(unsigned id) {
    switch (id) {
    case RETRO_MEMORY_SAVE_RAM:
        return gameboy.cartridge_slot.cartridge->get_ram_save_data();
    case RETRO_MEMORY_RTC:
        return nullptr;
    default:
        return nullptr;
    }
}

size_t retro_get_memory_size(unsigned id) {
    switch (id) {
    case RETRO_MEMORY_SAVE_RAM:
        return gameboy.cartridge_slot.cartridge->get_ram_save_size();
    case RETRO_MEMORY_RTC:
        return 0;
    default:
        return 0;
    }
}

void retro_cheat_reset(void) {
}

void retro_cheat_set(unsigned index, bool enabled, const char* code) {
}
