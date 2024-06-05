#ifndef BOOTROMHELPERS_H
#define BOOTROMHELPERS_H

template <typename T>
constexpr T if_bootrom_else(T y, T n) {
#ifdef ENABLE_BOOTROM
    return y;
#else
    return n;
#endif
}

#endif // BOOTROMHELPERS_H
