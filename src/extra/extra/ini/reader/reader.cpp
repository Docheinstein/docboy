#include "reader.h"

#include <iterator>
#include "utils/io.h"
#include "utils/strings.hpp"

IniReader::Result IniReader::parse(const std::string& filename) {
    // Read all file's lines
    bool ok;
    std::vector<std::string> lines = read_file_lines(filename, &ok);
    if (!ok)
        return {Result::Outcome::ErrorReadFailed};

    auto getKeyValue = [](const std::string& line) {
        std::vector<std::string> tokens;
        split(line, std::back_inserter(tokens), '=');

        std::vector<std::string> trimmedTokens;
        std::transform(tokens.begin(), tokens.end(), std::back_inserter(trimmedTokens), [](const std::string& token) {
            return trim(token);
        });
        return trimmedTokens;
    };

    // Read lines one by one
    uint32_t lineIndex {};

    for (const auto& line : lines) {
        lineIndex++;

        // Eventually skip comments
        bool skip {};
        for (const auto& commentPrefix : commentPrefixes) {
            if (starts_with(line, commentPrefix)) {
                skip = true;
                break;
            }
        }

        if (skip)
            continue;

        std::vector<std::string> tokens = getKeyValue(line);
        if (tokens.size() >= 2) {
            const std::string& key = tokens[0];
            const std::string& value = tokens[1];

            if (const auto it = properties.find(key); it != properties.end()) {
                if (!parsers[key](value, it->second)) {
                    // Failed to parse option
                    return {Result::Outcome::ErrorParseFailed, lineIndex};
                }
            }
        }
    }

    // Everything's ok
    return {Result::Outcome::Success, lineIndex};
}

void IniReader::addCommentPrefix(const std::string& prefix) {
    commentPrefixes.push_back(prefix);
}

void IniReader::addProperty(const std::string& name, std::string& option) {
    addProperty(name, option, [](const std::string& s) {
        return s;
    });
}
