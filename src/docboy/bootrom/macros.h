#ifndef BOOTROMMACROS_H
#define BOOTROMMACROS_H

#ifdef ENABLE_BOOTROM
#define IF_BOOTROM(statement) statement
#define IF_NOT_BOOTROM(statement)
#define IF_BOOTROM_ELSE(t, f) t
#else
#define IF_BOOTROM(statement)
#define IF_NOT_BOOTROM(statement) statement
#define IF_BOOTROM_ELSE(t, f) f
#endif

#endif // BOOTROMMACROS_H