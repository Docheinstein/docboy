#ifndef CORE_H
#define CORE_H

#include "docboy/common/macros.h"
#include "docboy/gameboy/gameboy.h"

#ifdef ENABLE_DEBUGGER
class DebuggerBackend;
#endif

class Core {
    DEBUGGABLE_CLASS()
    TESTABLE_CLASS()

public:
    explicit Core(GameBoy& gb);

    // Emulation
    void cycle();
    void frame();

    bool run_for_cycles(uint32_t cycles);

    // Load ROM
#ifdef ENABLE_BOOTROM
    void load_boot_rom(const std::string& filename);
#endif
    void load_rom(const std::string& filename);

    // Serial
    void attach_serial_link(ISerialEndpoint& endpoint) const;
    void detach_serial_link() const;

    // Input
    void set_key(Joypad::Key key, Joypad::KeyState state) const {
        gb.joypad.set_key_state(key, state);
    }

#ifdef ENABLE_AUDIO
    // Audio
    void set_audio_sample_callback(std::function<void(const Apu::AudioSample)>&& audio_callback) const;

    void set_audio_sample_rate(double sample_rate) const {
        gb.apu.set_sample_rate(sample_rate);
    }

    void set_audio_volume(double volume) const;
    void set_audio_high_pass_filter_enabled(bool enabled) const;
#endif

    // Rumble
    void set_rumble_callback(std::function<void(bool)>&& rumble_callback) const;

    // Save/Load RAM/RTC
    void save(void* data) const;
    void load(const void* data) const;
    bool can_save() const;
    uint32_t get_save_size() const;

    // Save/Load State
    void save_state(void* data) const;
    void load_state(const void* data);
    uint32_t get_state_size() const;

    // Reset
    void reset();

    // Debug
#ifdef ENABLE_DEBUGGER
    void attach_debugger(DebuggerBackend& debugger);
    void detach_debugger();
    bool is_asking_to_quit() const;
#endif

    GameBoy& gb;

private:
    void tick_t0() const;
    void tick_t1() const;
    void tick_t2() const;
    void tick_t3() const;

    Parcel parcelize_state() const;
    void unparcelize_state(Parcel&& parcel);

    uint64_t ticks {};

    uint16_t cycles_of_artificial_vblank {};

#ifdef ENABLE_DEBUGGER
    DebuggerBackend* debugger {};

    bool resetting {};
#endif
};

#endif // CORE_H