#ifndef UTILSASSERTS_H
#define UTILSASSERTS_H

#include "utils/exceptions.h"

#ifdef ENABLE_ASSERTS
#define ASSERT1(condition) (static_cast<bool>(condition) ? ((void)0) : FATALX(#condition))
#define ASSERT2(condition, message) (static_cast<bool>(condition) ? ((void)0) : FATALX(#condition + " : " + message))
#define ASSERTN(_2, _1, func, ...) func
#define ASSERT(...) ASSERTN(__VA_ARGS__, ASSERT2, ASSERT1)(__VA_ARGS__)
#define ASSERT_NO_ENTRY() FATALX("CHECK_NO_ENTRY failed")
#define ASSERT_CODE(code)                                                                                              \
    do                                                                                                                 \
    code while (0)
#else
#define ASSERT(...) ((void)(0))
#define ASSERT_NO_ENTRY() ((void)(0))
#define ASSERT_CODE(code) ((void)(0))
#endif

#endif // UTILSASSERTS_H