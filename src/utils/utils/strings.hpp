#ifndef STRINGS_HPP
#define STRINGS_HPP

#include "asserts.h"
#include <algorithm>
#include <string>
#include <vector>

template <typename It, typename Pred = decltype(isspace)>
void split(const std::string& s, It inserter, Pred predicate = isspace) {
    auto begin = s.begin();
    std::string::const_iterator token_begin;
    std::string::const_iterator token_end;
    do {
        token_begin = std::find_if_not(begin, s.end(), predicate);
        token_end = std::find_if(token_begin, s.end(), predicate);
        auto token = std::string(token_begin, token_end);
        if (!token.empty())
            *inserter = token;
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
        if (it + 1 != container.end())
            ss += sep;
    }
    return ss;
}

std::string trim(const std::string& s);

bool equals_ignore_case(const std::string& s1, const std::string& s2);

std::string lpad(const std::string& s, std::size_t length, char ch = ' ');
std::string rpad(const std::string& s, std::size_t length, char ch = ' ');

#endif // STRINGS_HPP