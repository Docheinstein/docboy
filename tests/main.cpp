#define CATCH_CONFIG_RUNNER

// ---- begin test cases ----

#include "unittests.h"

#ifdef ENABLE_CGB
#include "cgbtests.h"
#else
#include "dmgtests.h"
#endif

// ---- end test cases   ----

std::string boot_rom;

int main(int argc, char* argv[]) {
#ifdef ENABLE_BOOTROM
    const char* var = std::getenv("DOCBOY_BIOS");
    if (!var) {
        std::cerr << "Please set the boot ROM with with 'DOCBOY_BIOS' environment variable." << std::endl;
        return 1;
    }
    boot_rom = var;
#endif

    runner_log = [](const std::string& message) {
        UNSCOPED_INFO(message);
    };

    return Catch::Session().run(argc, argv);
}