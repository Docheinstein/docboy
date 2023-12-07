#ifndef TOKEN_H
#define TOKEN_H

#include <cstdint>
#include <string>
#include <utility>

namespace Tui {
struct Token {
    template <typename T>
    Token(T&& str, uint32_t size) :
        string(std::forward<T>(str)),
        size(size) {
    }

    explicit Token(char ch) :
        Token(std::string {ch}, 1) {
    }

    std::string string;
    uint32_t size {};
};
} // namespace Tui
#endif // TOKEN_H