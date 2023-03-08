#ifndef DEBUGGERREADABLE_H
#define DEBUGGERREADABLE_H

#include "core/memory/readable.h"

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


#endif // DEBUGGERREADABLE_H