#include "libretro.h"
#include "core/core.h"
#include "core/definitions.h"
#include "core/gameboy.h"
#include "utils/arrayutils.h"
#include <cstdarg>
#include <cstring>

static struct retro_log_callback logging;
static retro_log_printf_t retro_log;

static retro_environment_t environ_cb;
static retro_video_refresh_t video_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

// Use MAX_FREQUENCY since libretro frontend handles the emulation speed by itself
static GameBoy gameboy {GameBoy::Builder().setFrequency(IClock::MAX_FREQUENCY).build()};
static Core core(gameboy);

static void retro_fallback_log(enum retro_log_level level, const char* fmt, ...) {
    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
}

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
    info->library_name = "docboy";
    info->library_version = "0.1";
    info->need_fullpath = true;
    info->valid_extensions = "gb|gbc";
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

    if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
        retro_log = logging.log;
    else
        retro_log = retro_fallback_log;

    static const struct retro_controller_description controllers[] = {
        {"Nintendo DS", RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0)},
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

    static constexpr IJoypad::Key RETRO_KEYS_TO_GAMEBOY_KEYS[] = {
        IJoypad::Key::Left,  IJoypad::Key::Up,     IJoypad::Key::Down, IJoypad::Key::Right,
        IJoypad::Key::Start, IJoypad::Key::Select, IJoypad::Key::B,    IJoypad::Key::A,
    };

    input_poll_cb();

    for (uint32_t keyIndex = 0; keyIndex < array_size(RETRO_KEYS); keyIndex++) {
        uint16_t retroKey = RETRO_KEYS[keyIndex];
        const bool isKeyPressed = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, retroKey);
        core.setKey(RETRO_KEYS_TO_GAMEBOY_KEYS[keyIndex], isKeyPressed ? IJoypad::Pressed : IJoypad::Released);
    }

    // Tick until next frame
    core.frame();

    // Send framebuffer
    video_cb(gameboy.lcd.getFrameBuffer(), Specs::Display::WIDTH, Specs::Display::HEIGHT,
             Specs::Display::WIDTH * sizeof(uint16_t));
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
    core.loadROM(info->path);
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
    return 0;
}

bool retro_serialize(void* data_, size_t size) {
    return false;
}

bool retro_unserialize(const void* data_, size_t size) {
    return false;
}

void* retro_get_memory_data(unsigned id) {
    return nullptr;
}

size_t retro_get_memory_size(unsigned id) {
    return 0;
}

void retro_cheat_reset(void) {
}

void retro_cheat_set(unsigned index, bool enabled, const char* code) {
}
