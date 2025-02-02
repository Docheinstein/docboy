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

    // Load ROM
    void load_rom(const std::string& filename);

    // Serial
    void attach_serial_link(SerialLink::Plug& plug) const;

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

    void set_audio_volume(float volume) const {
        gb.apu.set_volume(volume);
    }
#endif

    // Save/Load RAM
    void save_ram(void* data) const;
    void load_ram(const void* data) const;
    bool can_save_ram() const;
    uint32_t get_ram_save_size() const;

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

    void _cycle();

    Parcel parcelize_state() const;
    void unparcelize_state(Parcel&& parcel);

    uint64_t ticks {};

#ifdef ENABLE_DEBUGGER
    DebuggerBackend* debugger {};

    bool resetting {};
#endif
};

#endif // CORE_H