#ifndef UTILSSTRINGS_H
#define UTILSSTRINGS_H

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "utils/asserts.h"

template <typename It, typename Pred = decltype(isspace)>
void split(const std::string& s, It inserter, Pred predicate = isspace) {
    auto begin = s.begin();
    std::string::const_iterator token_begin;
    std::string::const_iterator token_end;
    do {
        token_begin = std::find_if_not(begin, s.end(), predicate);
        token_end = std::find_if(token_begin, s.end(), predicate);
        auto token = std::string(token_begin, token_end);
        if (!token.empty()) {
            *inserter = token;
        }
        begin = token_end;
    } while (token_end != s.end());
}

template <typename It>
void split(const std::string& s, It inserter, char ch) {
    split(s, inserter, [ch](int c) {
        return c == ch;
    });
}

template <typename Container, typename SepType>
std::string join(const Container& container, SepType sep) {
    std::string ss;
    for (auto it = container.begin(); it != container.end(); it++) {
        ss += *it;
        if (it + 1 != container.end()) {
            ss += sep;
        }
    }
    return ss;
}

template <typename Container, typename SepType, typename Transform>
std::string join(const Container& container, SepType sep, Transform&& transform) {
    std::string ss;
    for (auto it = container.begin(); it != container.end(); it++) {
        ss += transform(*it);
        if (it + 1 != container.end()) {
            ss += sep;
        }
    }
    return ss;
}

void ltrim(std::string& s);
void rtrim(std::string& s);
void trim(std::string& s);

bool equals_ignore_case(const std::string& s1, const std::string& s2);

std::string lpad(const std::string& s, std::size_t length, char ch = ' ');
std::string rpad(const std::string& s, std::size_t length, char ch = ' ');

bool starts_with(const std::string& heystack, const std::string& needle);
bool ends_with(const std::string& heystack, const std::string& needle);

bool contains(const std::string& heystack, const std::string& needle);

template <uint8_t Precision, typename T>
std::string to_string_with_precision(const T value) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(Precision) << value;
    return ss.str();
}

std::optional<uint64_t> strtou(const std::string& s, uint8_t base = 10);
std::optional<int64_t> strtoi(const std::string& s, uint8_t base = 10);

#endif // UTILSSTRINGS_H
