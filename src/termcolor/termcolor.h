#ifndef TERMCOLOR_H
#define TERMCOLOR_H

#include <iosfwd>

namespace termcolor {

    template <uint8_t code, typename CharT>
    std::basic_ostream<CharT> & color(std::basic_ostream<CharT> &os) {
        os << "\033[38;5;" << +code << "m";
        return os;
    }

    template <uint8_t code, typename CharT>
    std::basic_ostream<CharT> & attr(std::basic_ostream<CharT> &os) {
        os << "\033[" << +code << "m";
        return os;
    }

    template <typename CharT>
    std::basic_ostream<CharT> & reset(std::basic_ostream<CharT> &os) {
        return attr<0>(os);
    }

    template <typename CharT>
    std::basic_ostream<CharT> & bold(std::basic_ostream<CharT> &os) {
        return attr<1>(os);
    }

    template <typename CharT>
    std::basic_ostream<CharT> & black(std::basic_ostream<CharT> &os) {
        return attr<30>(os);
    }

    template <typename CharT>
    std::basic_ostream<CharT> & red(std::basic_ostream<CharT> &os) {
        return attr<31>(os);
    }

    template <typename CharT>
    std::basic_ostream<CharT> & green(std::basic_ostream<CharT> &os) {
        return attr<32>(os);
    }

    template <typename CharT>
    std::basic_ostream<CharT> & yellow(std::basic_ostream<CharT> &os) {
        return attr<33>(os);
    }

    template <typename CharT>
    std::basic_ostream<CharT> & blue(std::basic_ostream<CharT> &os) {
        return attr<34>(os);
    }

    template <typename CharT>
    std::basic_ostream<CharT> & magenta(std::basic_ostream<CharT> &os) {
        return attr<35>(os);
    }

    template <typename CharT>
    std::basic_ostream<CharT> & cyan(std::basic_ostream<CharT> &os) {
        return attr<36>(os);
    }

    template <typename CharT>
    std::basic_ostream<CharT> & white(std::basic_ostream<CharT> &os) {
        return attr<37>(os);
    }

    template <typename CharT>
    std::basic_ostream<CharT> & bgblack(std::basic_ostream<CharT> &os) {
        return attr<40>(os);
    }

    template <typename CharT>
    std::basic_ostream<CharT> & bgred(std::basic_ostream<CharT> &os) {
        return attr<41>(os);
    }

    template <typename CharT>
    std::basic_ostream<CharT> & bggreen(std::basic_ostream<CharT> &os) {
        return attr<42>(os);
    }

    template <typename CharT>
    std::basic_ostream<CharT> & bgyellow(std::basic_ostream<CharT> &os) {
        return attr<43>(os);
    }

    template <typename CharT>
    std::basic_ostream<CharT> & bgblue(std::basic_ostream<CharT> &os) {
        return attr<44>(os);
    }

    template <typename CharT>
    std::basic_ostream<CharT> & bgmagenta(std::basic_ostream<CharT> &os) {
        return attr<45>(os);
    }

    template <typename CharT>
    std::basic_ostream<CharT> & bgcyan(std::basic_ostream<CharT> &os) {
        return attr<46>(os);
    }

    template <typename CharT>
    std::basic_ostream<CharT> & bgwhite(std::basic_ostream<CharT> &os) {
        return attr<47>(os);
    }
}

#endif // TERMCOLOR_H