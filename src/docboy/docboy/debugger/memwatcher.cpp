#include "docboy/debugger/memwatcher.h"

#include "docboy/core/core.h"
#include "docboy/debugger/backend.h"

namespace DebuggerMemoryWatcher {
namespace {
    DebuggerBackend* backend {};
}

void attach_backend(DebuggerBackend& b) {
    backend = &b;
}

void detach_backend() {
    backend = nullptr;
}

void notify_read(uint16_t address) {
    if (backend) {
        backend->notify_memory_read(address);
    }
}

void notify_write(uint16_t address) {
    if (backend) {
        backend->notify_memory_write(address);
    }
}
} // namespace DebuggerMemoryWatcher
