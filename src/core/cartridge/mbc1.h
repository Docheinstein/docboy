#ifndef MBC1_H
#define MBC1_H

#include "cartridge.h"

/*
 * In its default configuration, MBC1 supports up to 512 KiB ROM with up to 32 KiB of banked RAM.
 * Some cartridges wire the MBC differently, where the 2-bit RAM banking register
 * is wired as an extension of the ROM banking register (instead of to RAM)
 * in order to support up to 2 MiB ROM, at the cost of only supporting a
 * fixed 8 KiB of cartridge RAM. All MBC1 cartridges with 1 MiB of ROM or
 * more use this alternate wiring.
 */
class MBC1 : public Cartridge {
public:
    explicit MBC1(const std::vector<uint8_t> &data);
    explicit MBC1(std::vector<uint8_t> &&data);

    [[nodiscard]] uint8_t read(uint16_t address) const override;
    void write(uint16_t address, uint8_t value) override;

protected:
    struct {
        /*
         * 0000–1FFF — RAM Enable (Write Only)
         */
        bool ramEnabled;

        /*
         * 2000–3FFF — ROM Bank Number (Write Only)
         * ----------------------------------------
         * 5-bit register ($01-$1F) that selects the ROM bank number for the 4000–7FFF region
         */
        uint8_t romBankSelector;

        /*
         * 4000–5FFF — RAM Bank Number — or — Upper Bits of ROM Bank Number (Write Only)
         * -----------------------------------------------------------------------------
         * 2-bit register can be used to select
         * - RAM Bank in range from $00–$03 (32 KiB ram carts only)
         * - the upper two bits (bits 5-6)  of the ROM Bank number (>= 1 MiB ROM)
         */
        uint8_t upperRomBankSelector_ramBankSelector;

        /*
         * 6000–7FFF — Banking Mode Select (Write Only)
         * --------------------------------------------
         * 1-bit register selects between the two MBC1 banking modes,
         * controlling the behaviour of the secondary 2-bit banking register.
         *
         *  00h = ROM Banking Mode (up to 8KByte RAM, 2MByte ROM) (default)
         *  01h = RAM Banking Mode (up to 32KByte RAM, 512KByte ROM)
         */
        uint8_t bankingMode;
    } mbc;
};

#endif // MBC1_H