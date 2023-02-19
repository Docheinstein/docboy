#ifndef IO_H
#define IO_H

#include "memory.h"

class IO : public IMemory {
public:
    explicit IO();
    ~IO() override;

    [[nodiscard]] uint8_t read(size_t index) const override;
    void write(size_t index, uint8_t value) override;
    void reset() override;

    [[nodiscard]] uint8_t readSB() const;
    [[nodiscard]] uint8_t readSC() const;
    void writeSB(uint8_t value);
    void writeSC(uint8_t value);

private:
    uint8_t memory[256];
};

#endif // IO_H