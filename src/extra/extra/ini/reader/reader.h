#ifndef INIREADER_H
#define INIREADER_H

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

class IniReader {
public:
    struct Result {
        enum class Outcome {
            Success,
            ErrorReadFailed,
            ErrorParseFailed,
        };

        Outcome outcome {};
        uint32_t last_read_line {};
    };

    Result parse(const std::string& filename);

    template <typename T, typename P>
    void add_property(const std::string& name, T& target_option, P&& parser);

    void add_property(const std::string& name, std::string& option);

    void add_comment_prefix(const std::string& prefix);

private:
    std::map<std::string, void*> properties {};
    std::map<std::string, std::function<bool(const std::string&, void*)>> parsers {};

    std::vector<std::string> comment_prefixes {};
};

#include "extra/ini/reader/reader.tpp"

#endif // INIREADER_H
