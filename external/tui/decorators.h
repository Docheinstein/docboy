#ifndef DECORATORS_H
#define DECORATORS_H

#include "decorator.h"

namespace Tui {

template <uint8_t code>
Text color(Text&& text);

template <uint8_t code>
Text attr(Text&& text);

Text bold(Text&& text);

Text reset();

Text red(Text&& text);
Text lightred(Text&& text);
Text green(Text&& text);
Text lightgreen(Text&& text);
Text yellow(Text&& text);
Text lightyellow(Text&& text);
Text blue(Text&& text);
Text lightblue(Text&& text);
Text magenta(Text&& text);
Text lightmagenta(Text&& text);
Text cyan(Text&& text);
Text lightcyan(Text&& text);
Text gray(Text&& text);
Text lightgray(Text&& text);
Text darkgray(Text&& text);
Text verydarkgray(Text&& text);
} // namespace Tui

#include "decorators.tpp"

#endif // DECORATORS_H