#ifndef BOOT_H
#define BOOT_H

#include "bootrom.h"
#include "core/io/boot.h"
#include "core/state/processor.h"
#include <memory>

class Boot : public IBootROM, public IBootIO, public IStateProcessor {
public:
    explicit Boot(std::unique_ptr<IBootROM> bootRom = nullptr);

    void loadState(IReadableState& state) override;
    void saveState(IWritableState& state) override;

    [[nodiscard]] uint8_t read(uint16_t index) const override;

    [[nodiscard]] uint8_t readBOOT() const override;
    void writeBOOT(uint8_t value) override;

private:
    std::unique_ptr<IBootROM> bootRom;
    uint8_t BOOT;
};

#endif // BOOT_H