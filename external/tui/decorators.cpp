#include "decorators.h"

#define COLOR(c) Decorator("\033[38;5;" #c "m")
#define ATTR(c) Decorator("\033[" #c "m")
#define RESET() ATTR(0)
#define COLORIZE(c, t) COLOR(c) + t + RESET()
#define ATTRIBUTIZE(c, t) ATTR(c) + t + RESET()

namespace Tui {
Text bold(Text&& text) {
    return ATTRIBUTIZE(1, text);
}

Text reset() {
    return Text {} + RESET();
}

Text red(Text&& text) {
    return ATTRIBUTIZE(31, text);
}

Text lightred(Text&& text) {
    return COLORIZE(9, text);
}

Text green(Text&& text) {
    return ATTRIBUTIZE(32, text);
}

Text lightgreen(Text&& text) {
    return COLORIZE(10, text);
}

Text yellow(Text&& text) {
    return ATTRIBUTIZE(33, text);
}

Text lightyellow(Text&& text) {
    return COLORIZE(11, text);
}

Text blue(Text&& text) {
    return ATTRIBUTIZE(34, text);
}

Text lightblue(Text&& text) {
    return COLORIZE(12, text);
}

Text magenta(Text&& text) {
    return ATTRIBUTIZE(35, text);
}

Text lightmagenta(Text&& text) {
    return COLORIZE(13, text);
}

Text cyan(Text&& text) {
    return ATTRIBUTIZE(36, text);
}

Text lightcyan(Text&& text) {
    return COLORIZE(14, text);
}

Text gray(Text&& text) {
    return COLORIZE(244, text);
}

Text lightgray(Text&& text) {
    return COLORIZE(248, text);
}

Text darkgray(Text&& text) {
    return COLORIZE(240, text);
}

Text verydarkgray(Text&& text) {
    return COLORIZE(238, text);
}

} // namespace Tui