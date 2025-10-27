#include "docboy/debugger/symparser.h"

#include "utils/io.h"
#include "utils/strings.h"

std::vector<DebugSymbol> DebugSymbolsParser::parse_sym_file(const std::string& path) {
    std::vector<DebugSymbol> symbols;

    bool ok;
    std::vector<std::string> lines = read_file_lines(path, &ok);

    if (!ok) {
        return symbols;
    }

    // From rgbds:
    //
    // Comments begin with a semicolon.
    //
    // After removing the comment (if any), the line shall be split into non-empty tokens,
    // separated by one or more consecutive whitespace characters. Leading or trailing whitespace must be ignored.
    //
    // If any part of a line cannot be processed by the implementation, the whole line shall be ignored
    //
    // A location can have one of the following three forms:
    //   <number>:<number>, with each <number> indicating the location's bank and address respectively;
    //   BOOT:<number>, indicating an address in the boot ROM;
    //   <number>, indicating an address irrespective of bank, i.e. a location in every possible bank.
    for (auto& line : lines) {
        trim(line);
        if (starts_with(line, ";")) {
            // Skip comment
            continue;
        }

        std::vector<std::string> line_tokens {};
        split(line, std::back_inserter(line_tokens));

        if (line_tokens.size() != 2) {
            // Invalid line
            continue;
        }

        std::vector<std::string> address_specifier_tokens {};
        split(line_tokens[0], std::back_inserter(address_specifier_tokens), ':');
        if (address_specifier_tokens.size() != 2) {
            // Invalid line
            continue;
        }

        DebugSymbol symbol {};

        if (address_specifier_tokens[0] == "BOOT") {
            symbol.boot = true;
        } else {
            if (std::optional<uint64_t> bank = strtou(address_specifier_tokens[0], 16)) {
                symbol.bank = *bank;
            } else {
                // Invalid line
                continue;
            }
        }

        if (std::optional<uint64_t> addr = strtou(address_specifier_tokens[1], 16)) {
            symbol.address = *addr;
        } else {
            // Invalid line
            continue;
        }

        symbol.name = line_tokens[1];

        symbols.push_back(std::move(symbol));
    }

    return symbols;
}
