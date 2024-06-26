#include "strings.h"

namespace {
void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
}

void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         [](unsigned char ch) {
                             return !std::isspace(ch);
                         })
                .base(),
            s.end());
}
} // namespace

std::string trim(const std::string& s) {
    std::string out = s;
    ltrim(out);
    rtrim(out);
    return out;
}

bool equals_ignore_case(const std::string& s1, const std::string& s2) {
    return std::equal(s1.begin(), s1.end(), s2.begin(), [](const char& c1, const char& c2) {
        return std::tolower(c1) == std::tolower(c2);
    });
}

std::string lpad(const std::string& s, std::size_t length, char ch) {
    const std::size_t in_length = s.length();
    return length < in_length ? std::string(length - in_length, ch) + s : s;
}

std::string rpad(const std::string& s, std::size_t length, char ch) {
    const std::size_t in_length = s.length();
    return in_length < length ? s + std::string(length - in_length, ch) : s;
}

bool starts_with(const std::string& heystack, const std::string& needle) {
    return heystack.find(needle) == 0;
}
