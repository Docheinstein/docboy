#ifndef DEBUGGERJOYPADIO_H
#define DEBUGGERJOYPADIO_H

#include <cstddef>
#include <cstdint>

class IJoypadIODebug {
public:
    class Observer {
    public:
        virtual ~Observer() = default;

        virtual void onReadP1(uint8_t value) = 0;
        virtual void onWriteP1(uint8_t oldValue, uint8_t newValue) = 0;
    };

    virtual ~IJoypadIODebug() = default;

    virtual void setObserver(Observer *observer) = 0;
};


#endif // DEBUGGERJOYPADIO_H