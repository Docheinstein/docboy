#ifndef BOOTROMMACROS_H
#define BOOTROMMACROS_H

#ifdef ENABLE_BOOTROM
#define BOOTROM_ONLY(statement) statement
#else
#define BOOTROM_ONLY(statement)
#endif

#endif // BOOTROMMACROS_H