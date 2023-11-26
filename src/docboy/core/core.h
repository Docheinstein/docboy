#ifndef CORE_H
#define CORE_H

#include "docboy/debugger/macros.h"
#include "docboy/gameboy/gameboy.h"

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
    void saveState(void* data);
    void loadState(const void* data) const;
    [[nodiscard]] uint32_t getStateSaveSize() const;

    IF_DEBUGGER(void attachDebugger(DebuggerBackend& debugger));
    IF_DEBUGGER(void detachDebugger());
    IF_DEBUGGER(bool isDebuggerAskingToShutdown() const);

    GameBoy& gb;

    uint64_t ticks {};

    IF_DEBUGGER(DebuggerBackend* debugger {});

private:
    void tick_t0();
    void tick_t1();
    void tick_t2();
    void tick_t3();

    Parcel parcelizeState() const;
    void unparcelizeState(Parcel&& parcel) const;
};

#endif // CORE_H