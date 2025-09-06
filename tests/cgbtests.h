#ifndef CGBTESTS_H
#define CGBTESTS_H

#include "catch2/catch_all.hpp"

#include "runtests.h"

TEST_CASE("cgb", "[emulation]") {
    run_test_roms_from_json(TESTS_ROOT_FOLDER "/roms/cgb/", TESTS_ROOT_FOLDER "/results/cgb/",
                            TESTS_ROOT_FOLDER "/config/cgb.json");
}

#endif // CGBTESTS_H