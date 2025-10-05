#include "extra/ini/reader/reader.h"

#include <algorithm>
#include <iterator>

#include "utils/io.h"
#include "utils/strings.h"

IniReader::Result IniReader::parse(const std::string& filename) {
    // Read all file's lines
    bool ok;
    std::vector<std::string> lines = read_file_lines(filename, &ok);
    if (!ok) {
        return {Result::Outcome::ErrorReadFailed};
    }

    auto get_key_value = [](const std::string& line) {
        std::vector<std::string> tokens;
        split(line, std::back_inserter(tokens), '=');

        for (auto& token : tokens) {
            trim(token);
        }

        return tokens;
    };

    // Read lines one by one
    uint32_t line_index {};

    for (const auto& line : lines) {
        line_index++;

        // Eventually skip comments
        bool skip {};
        for (const auto& comment_prefix : comment_prefixes) {
            if (starts_with(line, comment_prefix)) {
                skip = true;
                break;
            }
        }

        if (skip) {
            continue;
        }

        std::vector<std::string> tokens = get_key_value(line);
        if (tokens.size() >= 2) {
            const std::string& key = tokens[0];
            const std::string& value = tokens[1];

            if (const auto it = properties.find(key); it != properties.end()) {
                if (!parsers[key](value, it->second)) {
                    // Failed to parse option
                    return {Result::Outcome::ErrorParseFailed, line_index};
                }
            }
        }
    }

    // Everything's ok
    return {Result::Outcome::Success, line_index};
}

void IniReader::add_comment_prefix(const std::string& prefix) {
    comment_prefixes.push_back(prefix);
}

void IniReader::add_property(const std::string& name, std::string& option) {
    add_property(name, option, [](const std::string& s) {
        return s;
    });
}
