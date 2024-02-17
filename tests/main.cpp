#define CATCH_CONFIG_RUNNER

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

// ---- begin test cases ----

#include "emutests.hpp"
#include "unittests.hpp"

// ---- end test cases   ----

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        return 1;

    IMG_Init(IMG_INIT_PNG);

    return Catch::Session().run(argc, argv);
}