#include "strutils.h"

bool equals_ignore_case(const std::string &s1, const std::string &s2) {
    return std::equal(s1.begin(), s1.end(), s2.begin(),
                      [](const char &c1, const char &c2) {
          return std::tolower(c1) == std::tolower(c2);
    });
}


static void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

static void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

std::string trim(const std::string &s) {
    std::string out = s;
    ltrim(out);
    rtrim(out);
    return out;
}
