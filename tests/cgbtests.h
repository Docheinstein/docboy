#ifndef CGBTESTS_H
#define CGBTESTS_H

#include "catch2/catch_all.hpp"

#include "runtests.h"

TEST_CASE("cgb", "[emulation]") {
    run_test_roms_from_json(TESTS_ROOT_FOLDER "/roms/cgb/", TESTS_ROOT_FOLDER "/results/cgb/",
                            TESTS_ROOT_FOLDER "/config/cgb.json");
}

TEST_CASE("cgb_dmg_mode", "[emulation]") {
    run_test_roms_from_json(TESTS_ROOT_FOLDER "/roms/cgb_dmg_mode/", TESTS_ROOT_FOLDER "/results/cgb_dmg_mode/",
                            TESTS_ROOT_FOLDER "/config/cgb_dmg_mode.json");
}

#endif // CGBTESTS_H