#ifndef DEBUGGERMEMORY_H
#define DEBUGGERMEMORY_H

#include "core/memory/memory.h"
#include "readable.h"

class IDebuggableWritable : public virtual IWritable {
public:
    class Observer {
    public:
        virtual ~Observer() = default;
        virtual void onWrite(uint16_t addr, uint8_t oldValue, uint8_t newValue) = 0;
    };

    ~IDebuggableWritable() override = default;

    virtual void setObserver(Observer *observer) = 0;

    virtual void writeRaw(uint16_t addr, uint8_t value) = 0;
};

class IDebuggableMemory : public virtual IDebuggableReadable, public virtual IDebuggableWritable {
public:
    class Observer : public IDebuggableReadable::Observer, public IDebuggableWritable::Observer {};

    void setObserver(IDebuggableReadable::Observer *observer) override = 0;
    void setObserver(IDebuggableWritable::Observer *observer) override = 0;
    virtual void setObserver(IDebuggableMemory::Observer *observer) = 0;

    ~IDebuggableMemory() override = default;
};

class DebuggableMemory : public IDebuggableMemory, public Memory {
public:
    explicit DebuggableMemory(size_t size);
    explicit DebuggableMemory(const std::vector<uint8_t> &data);
    explicit DebuggableMemory(std::vector<uint8_t> &&data);

    void setObserver(IDebuggableReadable::Observer *o) override;
    void setObserver(IDebuggableWritable::Observer *o) override;
    void setObserver(Observer *o) override;

    [[nodiscard]] uint8_t read(uint16_t index) const override;
    void write(uint16_t index, uint8_t value) override;

    [[nodiscard]] uint8_t readRaw(uint16_t index) const override;
    void writeRaw(uint16_t index, uint8_t value) override;

private:
    IDebuggableReadable::Observer *robserver;
    IDebuggableWritable::Observer *wobserver;
};



#endif // DEBUGGERMEMORY_H