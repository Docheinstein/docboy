#ifndef LOG_H
#define LOG_H

#ifdef ENABLE_LOGGING
#include <iostream>
#define ERROR(message) std::cout << "ERROR: " << message << std::endl;
#define WARN(message) std::cout << "WARN: " << message << std::endl;
#else
#define ERROR(message)
#define WARN(message)
#endif

#endif // LOG_H