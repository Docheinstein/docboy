#define CATCH_CONFIG_RUNNER

// ---- begin test cases ----

#include "emutests.hpp"
#include "unittests.hpp"

// ---- end test cases   ----

int main(int argc, char* argv[]) {
    return Catch::Session().run(argc, argv);
}