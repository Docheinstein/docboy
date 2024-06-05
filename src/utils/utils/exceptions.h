#ifndef UTILSEXCEPTIONS_H
#define UTILSEXCEPTIONS_H

#ifdef __FILE_NAME__
#define FILENAME __FILE_NAME__
#else
#define FILENAME __FILE__
#endif

#if __cpp_exceptions
#include <stdexcept>
#include <string>
#define FATAL(message) throw std::runtime_error(std::string() + message)
#define FATALX(message) FATAL(FILENAME + "@" + __func__ + ":" + std::to_string(__LINE__) + "   " + message)
#else
#include <cstdlib>
#include <iostream>
#define FATAL(message)                                                                                                 \
    do {                                                                                                               \
        std::cerr << message << std::endl;                                                                             \
        abort();                                                                                                       \
    } while (0)
#define FATALX(message)                                                                                                \
    do {                                                                                                               \
        std::cerr << FILENAME << "@" << __func__ << ":" << std::to_string(__LINE__) << "   " << message << std::endl;  \
        abort();                                                                                                       \
    } while (0)
#endif

#ifndef _EXCEPTION_DEFINES_H
#define _EXCEPTION_DEFINES_H 1

#if !__cpp_exceptions
// Iff -fno-exceptions, transform error handling code to work without it.
#define __try if (true)
#define __catch(X) if (false)
#define __throw_exception_again
#else
// Else proceed normally.
#define __try try
#define __catch(X) catch (X)
#define __throw_exception_again throw
#endif

#endif

#endif // UTILSEXCEPTIONS_H