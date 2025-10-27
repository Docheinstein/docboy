#ifndef SYMPARSER_H
#define SYMPARSER_H

#include <vector>

#include "docboy/debugger/shared.h"

namespace DebugSymbolsParser {
std::vector<DebugSymbol> parse_sym_file(const std::string& path);
};

#endif // SYMPARSER_H