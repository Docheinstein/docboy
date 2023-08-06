#ifndef DEBUGGERREADABLE_H
#define DEBUGGERREADABLE_H

#include "core/memory/readable.h"
#include <string>

class IReadableDebug {
public:
    class Observer {
    public:
        virtual ~Observer() = default;
        virtual void onRead(uint16_t addr, uint8_t value) = 0;
        virtual void onReadError(uint16_t addr, const std::string& error) = 0;
    };
    virtual ~IReadableDebug() = default;

    virtual void setObserver(Observer* observer) = 0;
};

class DebuggableReadOnlyMemory : public IReadableDebug, public ReadOnlyMemory {
public:
    explicit DebuggableReadOnlyMemory(size_t size);
    explicit DebuggableReadOnlyMemory(const std::vector<uint8_t>& data);
    explicit DebuggableReadOnlyMemory(std::vector<uint8_t>&& data);

    void setObserver(Observer* o) override;

    [[nodiscard]] uint8_t read(uint16_t index) const override;

private:
    Observer* observer;
};

#endif // DEBUGGERREADABLE_H