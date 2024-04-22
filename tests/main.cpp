#define CATCH_CONFIG_RUNNER

// ---- begin test cases ----

#include "emutests.hpp"
#include "unittests.hpp"

// ---- end test cases   ----

std::string bootRom;

int main(int argc, char* argv[]) {
#ifdef ENABLE_BOOTROM
    const char* var = std::getenv("DOCBOY_DMG_BIOS");
    if (!var) {
        std::cerr << "Please set the boot ROM with with 'DOCBOY_DMG_BIOS' environment variable." << std::endl;
        return 1;
    }
    bootRom = var;
#endif

    return Catch::Session().run(argc, argv);
}