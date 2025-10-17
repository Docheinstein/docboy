#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <cstdint>
#include <functional>

class Parcel;

class ICartridge {
public:
    virtual ~ICartridge() = default;

    virtual uint8_t read_rom(uint16_t address) const = 0;
    virtual void write_rom(uint16_t address, uint8_t value) = 0;

    virtual uint8_t read_ram(uint16_t address) const = 0;
    virtual void write_ram(uint16_t address, uint8_t value) = 0;

    void tick() {
        if (need_ticks) {
            on_tick();
        }
    }

    virtual void set_rumble_callback(std::function<void(bool)>&& rumble_callback) {
    }

    virtual void* get_ram_save_data() = 0;
    virtual uint32_t get_ram_save_size() const = 0;

    virtual void* get_rtc_save_data() {
        return nullptr;
    }
    virtual uint32_t get_rtc_save_size() const {
        return 0;
    }

#ifdef ENABLE_DEBUGGER
    virtual uint8_t* get_rom_data() = 0;
    virtual uint32_t get_rom_size() const = 0;
#endif

    virtual void save_state(Parcel& parcel) const = 0;
    virtual void load_state(Parcel& parcel) = 0;

    virtual void reset() = 0;

protected:
    virtual void on_tick() {
    }

    bool need_ticks {};
};

#endif // CARTRIDGE_H