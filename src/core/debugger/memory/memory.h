#ifndef DEBUGGERMEMORY_H
#define DEBUGGERMEMORY_H

#include "core/memory/memory.h"
#include "readable.h"

class IWritableDebug {
public:
    class Observer {
    public:
        virtual ~Observer() = default;
        virtual void onWrite(uint16_t addr, uint8_t oldValue, uint8_t newValue) = 0;
    };

    virtual ~IWritableDebug() = default;

    virtual void setObserver(Observer* observer) = 0;
};

class IMemoryDebug : public IReadableDebug, public IWritableDebug {
public:
    class Observer : public IReadableDebug::Observer, public IWritableDebug::Observer {};

    using IReadableDebug::setObserver;
    using IWritableDebug::setObserver;
    virtual void setObserver(IMemoryDebug::Observer* observer) = 0;

    ~IMemoryDebug() override = default;
};

class DebuggableMemory : public IMemoryDebug, public Memory {
public:
    explicit DebuggableMemory(size_t size);
    explicit DebuggableMemory(const std::vector<uint8_t>& data);
    explicit DebuggableMemory(std::vector<uint8_t>&& data);

    void setObserver(IReadableDebug::Observer* o) override;
    void setObserver(IWritableDebug::Observer* o) override;
    void setObserver(Observer* o) override;

    [[nodiscard]] uint8_t read(uint16_t index) const override;
    void write(uint16_t index, uint8_t value) override;

private:
    IReadableDebug::Observer* robserver;
    IWritableDebug::Observer* wobserver;
};

#endif // DEBUGGERMEMORY_H