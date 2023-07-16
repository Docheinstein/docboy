#ifndef DEBUGGERBOOT_H
#define DEBUGGERBOOT_H

#include "core/boot/boot.h"
#include "core/debugger/io/boot.h"
#include "core/debugger/memory/readable.h"

class DebuggableBoot : public IBootIODebug, public IReadableDebug, public Boot {
public:
    explicit DebuggableBoot(std::unique_ptr<IBootROM> bootRom = nullptr);

    [[nodiscard]] uint8_t read(uint16_t index) const override;

    [[nodiscard]] uint8_t readBOOT() const override;
    void writeBOOT(uint8_t value) override;

    void setObserver(IReadableDebug::Observer* o) override;
    void setObserver(IBootIODebug::Observer* o) override;

private:
    IReadableDebug::Observer* romObserver;
    IBootIODebug::Observer* ioObserver;
};

#endif // DEBUGGERBOOT_H