#ifndef LOG_H
#define LOG_H

#include <iostream>

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 0
#endif

#define DEBUG_LV(lv) \
if (lv <= DEBUG_LEVEL) \
    debug

#define DEBUG_1() DEBUG_LV(1)
#define DEBUG_PICKER(_1, FUNC, ...) FUNC
#define DEBUG(...) DEBUG_PICKER(__VA_OPT__(,) DEBUG_LV, DEBUG_1)(__VA_ARGS__)

#ifndef WARN
#define WARN() std::cout << "WARNING: "
#endif

extern std::ostream &debug;

#endif // LOG_H