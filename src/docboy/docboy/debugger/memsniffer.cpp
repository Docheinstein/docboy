#include "docboy/debugger/memsniffer.h"

#include "docboy/debugger/backend.h"

namespace DebuggerMemorySniffer {
namespace {
    DebuggerBackend* observer {};
}

void notify_memory_read(uint16_t address, uint8_t value) {
    if (observer) {
        observer->notify_memory_read(address, value);
    }
}

void notify_memory_write(uint16_t address, uint8_t old_value, uint8_t new_value) {
    if (observer) {
        observer->notify_memory_write(address, old_value, new_value);
    }
}

void set_observer(DebuggerBackend* o) {
    observer = o;
}
} // namespace DebuggerMemorySniffer
