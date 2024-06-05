#ifndef MEMSNIFFER_H
#define MEMSNIFFER_H

#include <cstdint>

class DebuggerBackend;

namespace DebuggerMemorySniffer {
void set_observer(DebuggerBackend* observer);

void notify_memory_read(uint16_t address, uint8_t value);
void notify_memory_write(uint16_t address, uint8_t old_value, uint8_t new_value);
}; // namespace DebuggerMemorySniffer

#endif // MEMSNIFFER_H