#ifndef DEBUGGABLEMEMORY_H
#define DEBUGGABLEMEMORY_H

#include "core/memory.h"

class IDebuggableReadable : public virtual IReadable {
public:
    class Observer {
    public:
        virtual ~Observer() = default;
        virtual void onRead(uint16_t addr, uint8_t value) = 0;
    };
    ~IDebuggableReadable() override = default;

    virtual void setObserver(Observer *observer) = 0;

    [[nodiscard]] virtual uint8_t readRaw(uint16_t addr) const = 0;
};

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

class IDebuggableMemory : public virtual IMemory {
public:
//    class Observer {
//    public:
//        virtual ~Observer() = default;
//        virtual void onRead(uint16_t addr, uint8_t value) = 0;
//        virtual void onWrite(uint16_t addr, uint8_t oldValue, uint8_t newValue) = 0;
//    };
    class Observer : public IDebuggableReadable::Observer, public IDebuggableWritable::Observer {};

    ~IDebuggableMemory() override = default;

    virtual void setObserver(Observer *observer) = 0;

    [[nodiscard]] virtual uint8_t readRaw(uint16_t addr) const = 0;
    virtual void writeRaw(uint16_t addr, uint8_t value) = 0;
};

class DebuggableReadable : public IDebuggableReadable, public Readable {
public:
    explicit DebuggableReadable(size_t size);
    explicit DebuggableReadable(const std::vector<uint8_t> &data);
    explicit DebuggableReadable(std::vector<uint8_t> &&data);

    void setObserver(Observer *o) override;

    [[nodiscard]] uint8_t read(uint16_t index) const override;
    [[nodiscard]] uint8_t readRaw(uint16_t index) const override;

private:
    Observer *observer;
};

class DebuggableMemory : public IDebuggableMemory, public Memory {
public:
    explicit DebuggableMemory(size_t size);
    explicit DebuggableMemory(const std::vector<uint8_t> &data);
    explicit DebuggableMemory(std::vector<uint8_t> &&data);

    void setObserver(Observer *o) override;

    [[nodiscard]] uint8_t read(uint16_t index) const override;
    void write(uint16_t index, uint8_t value) override;

    [[nodiscard]] uint8_t readRaw(uint16_t index) const override;
    void writeRaw(uint16_t index, uint8_t value) override;

private:
    Observer *observer;
};


#endif // DEBUGGABLEMEMORY_H