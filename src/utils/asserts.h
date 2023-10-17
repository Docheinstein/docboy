#ifndef assertS_H
#define assertS_H

#include "exceptions.hpp"

#ifdef ENABLE_ASSERTS
#define check1(condition) (static_cast<bool>(condition) ? ((void)0) : fatalx(#condition))
#define check2(condition, message) (static_cast<bool>(condition) ? ((void)0) : fatalx(#condition + " : " + message))
#define checkN(_2, _1, func, ...) func

#define check(...) checkN(__VA_ARGS__, check2, check1)(__VA_ARGS__)
#define checkNoEntry() fatalx("checkNoEntry failed")
#define checkCode(code)                                                                                                \
    do {                                                                                                               \
        code                                                                                                           \
    } while (0)
#define IF_ASSERTS(statement) statement
#else
#define check(...) ((void)(0))
#define checkNoEntry() ((void)(0))
#define checkCode(code) ((void)(0))
#define IF_ASSERTS(statement)
#endif

#endif // assertS_H