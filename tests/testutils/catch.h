#ifndef CATCH_H
#define CATCH_H

#include "catch2/catch_all.hpp"

#define TABLE_1(t1, data) GENERATE(table<t1> data)
#define TABLE_2(t1, t2, data) GENERATE(table<t1, t2> data)
#define TABLE_3(t1, t2, t3, data) GENERATE(table<t1, t2, t3> data)
#define TABLE_4(t1, t2, t3, t4, data) GENERATE(table<t1, t2, t3, t4> data)

#define TABLE_PICKER(_1, _2, _3, _4, _5, FUNC, ...) FUNC
#define TABLE(...) TABLE_PICKER(__VA_ARGS__, TABLE_4, TABLE_3, TABLE_2, TABLE_1)(__VA_ARGS__)

#endif // CATCH_H