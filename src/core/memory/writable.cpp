#include "writable.h"

WriteMemoryException::WriteMemoryException(const std::string& what) {
    error = what;
}

const char* WriteMemoryException::what() const noexcept {
    return error.c_str();
}
