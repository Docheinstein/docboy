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
    void tick();
    void frame();

    // Load ROM
    void loadRom(const std::string& filename) const;
    void loadRom(std::unique_ptr<ICartridge>&& cartridge) const;

    // Serial
    void attachSerialLink(SerialLink::Plug& plug) const;

    // Input
    void setKey(Joypad::Key key, Joypad::KeyState state) const;

    // Save/Load RAM
    void saveRam(void* data) const;
    void loadRam(const void* data) const;
    [[nodiscard]] bool canSaveRam() const;
    [[nodiscard]] uint32_t getRamSaveSize() const;

    // Save/Load State
    void saveState(void* data) const;
    void loadState(const void* data) const;
    [[nodiscard]] uint32_t getStateSaveSize() const;

    DEBUGGER_ONLY(void attachDebugger(DebuggerBackend& debugger));
    DEBUGGER_ONLY(void detachDebugger());
    DEBUGGER_ONLY(bool isDebuggerAskingToShutdown() const);

    GameBoy& gb;

    uint64_t ticks {};

    DEBUGGER_ONLY(DebuggerBackend* debugger {});

private:
    Parcel parcelizeState() const;
    void unparcelizeState(Parcel&& parcel) const;
};

#include "core.tpp"

#endif // CORE_H