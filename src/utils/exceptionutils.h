#ifndef EXCEPTIONUTILS_H
#define EXCEPTIONUTILS_H

#if __cpp_exceptions
#include <stdexcept>
#define THROW(exception) throw exception
#else
#include <cstdlib>
#define THROW(exception) abort()
#endif

// TODO: this should not be necessary, but bits/exception_defines.h sometimes is not found
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

#endif // EXCEPTIONUTILS_H