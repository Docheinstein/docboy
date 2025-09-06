#ifndef PERFORMANCE_H
#define PERFORMANCE_H

#include <chrono>
#include <iostream>

class Perf {
public:
    explicit Perf(const char* label = nullptr) :
        label {label},
        start {std::chrono::high_resolution_clock::now()} {
    }

    ~Perf() {
        const auto end = std::chrono::high_resolution_clock::now();
        if (label) {
            std::cout << "[" << label << "] ";
        }
        std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() << " ns" << std::endl;
    }

private:
    const char* label;
    std::chrono::high_resolution_clock::time_point start;
};

#endif