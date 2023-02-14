#ifndef STRUTILS_H
#define STRUTILS_H

#include <string>
#include <algorithm>

template<typename It>
void split(const std::string &s, It inserter, int (*predicate)(int) = std::isspace) {
    auto begin = s.begin();
    std::string::const_iterator token_begin;
    std::string::const_iterator token_end;
    do {
        token_begin = std::find_if_not(begin, s.end(), [&predicate](const char &c) {
            return predicate(c);
        });
        token_end = std::find_if(token_begin, s.end(), [&predicate](const char &c) {
            return predicate(c);
        });

        auto token = std::string(token_begin, token_end);
        if (!token.empty())
            *inserter = token;
        begin = token_end;
    } while (token_end != s.end());
}

#endif // STRUTILS_H