namespace Tui {
template <uint8_t code>
Text color(Text&& text) {
    return Decorator {"\033[38;5;" + std::to_string(code) + "m"} + text + "\033[0m";
}

template <uint8_t code>
Text attr(Text&& text) {
    return Decorator {"\033[" + std::to_string(code) + "m"} + text + "\033[0m";
}
} // namespace Tui