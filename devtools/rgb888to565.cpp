#include <iostream>

#include "extra/img/imgmanip.h"
#include "utils/formatters.h"

// usage: rgb888to565 <rgb888>

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: rgb888to565 <rgb888>" << std::endl;
        exit(1);
    }

    uint32_t rgb888 = std::stol(argv[1], nullptr, 16);

    std::cout << hex<uint16_t>(convert_pixel(rgb888, ImageFormat::RGB888, ImageFormat::RGB565)) << std::endl;

    return 0;
}