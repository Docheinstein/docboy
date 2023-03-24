#ifndef BOOT_H
#define BOOT_H

#include <memory>
#include "core/io/boot.h"
#include "bootrom.h"

class Boot : public IBootROM, public IBootIO {
public:
    explicit Boot(std::unique_ptr<IBootROM> bootRom = nullptr);

    [[nodiscard]] uint8_t read(uint16_t index) const override;

    [[nodiscard]] uint8_t readBOOT() const override;
    void writeBOOT(uint8_t value) override;

private:
    std::unique_ptr<IBootROM> bootRom;
    uint8_t BOOT;
};


#endif // BOOT_H