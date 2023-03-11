#include "helpers.h"
#include <SDL.h>
#include "utils/log.h"

bool screenshot(uint32_t *pixels, int width, int height, int format, const std::string &filename) {
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom(
            pixels, width, height, SDL_BITSPERPIXEL(format), width * SDL_BYTESPERPIXEL(format), format);
    if (!surface) {
        WARN() << "SDL_CreateRGBSurfaceWithFormatFrom error: " << SDL_GetError() << std::endl;
        return false;
    }

    int result = SDL_SaveBMP(surface, filename.c_str());

    SDL_FreeSurface(surface);

    if (result != 0)
        WARN() << "SDL_SaveBMP error: " << SDL_GetError() << std::endl;

    return result == 0;
}
