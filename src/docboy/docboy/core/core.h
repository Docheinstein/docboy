#ifndef CORE_H
#define CORE_H

#include "docboy/gameboy/gameboy.h"
#include "docboy/shared//macros.h"

#ifdef ENABLE_DEBUGGER
class DebuggerBackend;
#endif

class Core {
    DEBUGGABLE_CLASS()

public:
    explicit Core(GameBoy& gb);

    // Emulation
    void cycle();
    void frame();

    // Load ROM
    void loadRom(const std::string& filename) const;
    void loadRom(std::unique_ptr<ICartridge>&& cartridge) const;

    // Serial
    void attachSerialLink(SerialLink::Plug& plug) const;

    // Input
    void setKey(Joypad::Key key, Joypad::KeyState state) const {
        gb.joypad.setKeyState(key, state);
    }

    // Save/Load RAM
    void saveRam(void* data) const;
    void loadRam(const void* data) const;
    [[nodiscard]] bool canSaveRam() const;
    [[nodiscard]] uint32_t getRamSaveSize() const;

    // Save/Load State
    void saveState(void* data) const;
    void loadState(const void* data);
    [[nodiscard]] uint32_t getStateSize() const;

    IF_DEBUGGER(void attachDebugger(DebuggerBackend& debugger));
    IF_DEBUGGER(void detachDebugger());
    IF_DEBUGGER(bool isDebuggerAskingToShutdown() const);

    GameBoy& gb;

    uint64_t ticks {};

    IF_DEBUGGER(DebuggerBackend* debugger {});

private:
    void tick_t0() const;
    void tick_t1() const;
    void tick_t2() const;
    void tick_t3() const;

    [[nodiscard]] Parcel parcelizeState() const;
    void unparcelizeState(Parcel&& parcel);

    void saveState(Parcel& parcel) const;
    void loadState(Parcel& parcel);
};

#endif // CORE_H