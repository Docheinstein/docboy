#ifndef MEMWATCHER_H
#define MEMWATCHER_H

#include <cstdint>

class DebuggerBackend;

namespace DebuggerMemoryWatcher {
void set_observer(DebuggerBackend* observer);

void notify_read(uint16_t address, uint8_t value);
void notify_write(uint16_t address, uint8_t old_value, uint8_t new_value);
}; // namespace DebuggerMemoryWatcher

#endif // MEMWATCHER_H