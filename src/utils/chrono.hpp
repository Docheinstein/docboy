#ifndef CHRONO_HPP
#define CHRONO_HPP

#include <chrono>
#include <ctime>

std::tm get_current_datetime() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    return *std::localtime(&time);
}

#endif // CHRONO_HPP