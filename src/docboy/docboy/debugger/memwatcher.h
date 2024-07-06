#ifndef MEMWATCHER_H
#define MEMWATCHER_H

#include <cstdint>

class DebuggerBackend;

namespace DebuggerMemoryWatcher {
void attach_backend(DebuggerBackend& b);
void detach_backend();

void notify_read(uint16_t address);
void notify_write(uint16_t address);
}; // namespace DebuggerMemoryWatcher

#endif // MEMWATCHER_H