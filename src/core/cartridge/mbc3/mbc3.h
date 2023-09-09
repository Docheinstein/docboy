#ifndef MBC3_H
#define MBC3_H

#include "core/cartridge/cartridge.h"

/*
 * Beside for the ability to access up to 2MB ROM (128 banks), and 32KB RAM (4 banks),
 * the MBC3 also includes a built-in Real Time Clock (RTC).
 * The RTC requires an external 32.768 kHz Quartz Oscillator,
 * and an external battery (if it should continue to tick when the Game Boy is turned off).
 *
 * (max 2MByte ROM and/or 32KByte RAM and Timer)
 */
class MBC3 : public Cartridge, public IStateProcessor {
public:
    explicit MBC3(const std::vector<uint8_t>& data);
    explicit MBC3(std::vector<uint8_t>&& data);

    void loadState(IReadableState& state) override;
    void saveState(IWritableState& state) override;

    [[nodiscard]] uint8_t read(uint16_t address) const override;
    void write(uint16_t address, uint8_t value) override;

protected:
    /*
     * 0000-1FFF - RAM and Timer Enable (Write Only)
     */
    bool ramAndTimerEnabled {};

    /*
     * 2000–3FFF — ROM Bank Number (Write Only)
     * ----------------------------------------
     * 5-bit register ($01-$1F) that selects the ROM bank number for the 4000–7FFF region
     */
    uint8_t romBankSelector {};

    /*
     * 4000-5FFF - RAM Bank Number - or - RTC Register Select (Write Only)
     * -----------------------------------------------------------------------------
     * - $00–$03 : select RAM Bank (32 KiB ram carts only)
     * - $00-$08 : map the corresponding RTC register into memory at A000-BFFF
     */
    uint8_t ramBankSelector_rtcRegisterSelector {};

    /*
     * 6000-7FFF - Latch Clock Data (Write Only)
     * -----------------------------------------------------------------------------
     * When writing $00, and then $01 to this register,
     * the current time becomes latched into the RTC registers.
     */
    uint8_t latchClockData {};

    struct {
        uint8_t seconds;
        uint8_t minutes;
        uint8_t hours;
        struct {
            uint8_t low;
            uint8_t high;
        } day;
    } rtcRegisters {};

    uint8_t* rtcRegistersMap[5];
};

#endif // MBC3_H