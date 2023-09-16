#include "iniutils.h"
#include "ioutils.h"
#include "strutils.h"
#include <regex>
#include <vector>

static std::regex SECTION_REGEX("\\[(.*)\\]");

static std::pair<std::string, std::string> split_key_value(const std::string& line) {
    std::vector<std::string> tokens;
    split(line, std::back_inserter(tokens), '=');

    std::vector<std::string> trimmedTokens;
    std::transform(tokens.begin(), tokens.end(), std::back_inserter(trimmedTokens), [](const std::string& token) {
        return trim(token);
    });
    return std::make_pair(trimmedTokens[0], trimmedTokens[1]);
}

INI ini_read(const std::string& filename, bool* ok, std::string* error) {
    INI ini;

    std::vector<std::string> lines = read_file_lines(filename, ok);
    if (ok && !(*ok)) {
        if (error)
            *error = "failed to read file";
        return ini;
    }

    std::string sectionName;
    INISection section;

    for (const auto& rawLine : lines) {
        const auto line = trim(rawLine);

        if (line.empty())
            continue;

        std::smatch match;
        if (std::regex_search(line.begin(), line.end(), match, SECTION_REGEX)) {
            // new section found
            if (!sectionName.empty() && !section.empty()) {
                ini[sectionName] = section;
            }
            sectionName = match.str(1);
            section.clear();
            continue;
        }

        const auto& [key, value] = split_key_value(line);
        section[key] = value;
    }

    // emplace last section
    if (!sectionName.empty() && !section.empty()) {
        ini[sectionName] = section;
    }

    return ini;
}

void ini_write(const std::string& filename, const INI& ini, bool* ok, std::string* error) {
    std::vector<std::string> lines;
    for (const auto& [section, content] : ini) {
        lines.push_back("[" + section + "]");
        for (const auto& [key, value] : content) {
            std::string line = key;
            line += " = ";
            line += value;
            lines.emplace_back(line);
        }
        lines.emplace_back("\n");
    }

    write_file_lines(filename, lines, ok);

    if (ok && !(*ok)) {
        if (error)
            *error = "failed to write file";
    }
}
