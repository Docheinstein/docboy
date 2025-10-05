#ifndef DMGTESTS_H
#define DMGTESTS_H

#include "catch2/catch_all.hpp"

#include "runtests.h"

TEST_CASE("dmg", "[emulation]") {
    run_test_roms_from_json(TESTS_ROOT_FOLDER "/roms/dmg/", TESTS_ROOT_FOLDER "/results/dmg/",
                            TESTS_ROOT_FOLDER "/config/dmg.json", "ppu");
}

#endif // DMGTESTS_H