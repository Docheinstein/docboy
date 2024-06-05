#include "extra/ini/writer/writer.h"

#include "utils/io.h"

bool IniWriter::write(const std::map<std::string, std::string>& properties, const std::string& filename) const {
    // Build lines as <key>=<value>
    std::vector<std::string> lines {};
    for (const auto& [k, v] : properties) {
        std::string l {};
        l += k;
        l += "=";
        l += v;
        lines.push_back(std::move(l));
    }

    // Write to file
    bool ok;
    write_file_lines(filename, lines, &ok);
    return ok;
}
