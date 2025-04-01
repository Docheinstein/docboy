#include <iostream>

#include "extra/img/imgmanip.h"
#include "utils/formatters.h"

// usage: rgb565to888 <rgb565>

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: rgb565to888 <rgb565>" << std::endl;
        exit(1);
    }

    uint32_t rgb565 = std::stol(argv[1], nullptr, 16);

    std::cout << hex(convert_pixel(rgb565, ImageFormat::RGB565, ImageFormat::RGB888)) << std::endl;

    return 0;
}