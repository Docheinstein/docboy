#include "img.h"
#include "hexdump.hpp"
#include <SDL_image.h>
#include <SDL_surface.h>
#include <iostream>

void load_png(const std::string& filename, void* buffer, int format, uint32_t* size) {
    SDL_Surface* surface = IMG_Load(filename.c_str());
    if (!surface)
        goto error;

    if (format != -1) {
        SDL_Surface* convertedSurface = SDL_ConvertSurfaceFormat(surface, format, 0);
        if (!convertedSurface)
            goto error;
        surface = convertedSurface;
    }

    if (size)
        *size = surface->h * surface->pitch;

    memcpy(buffer, surface->pixels, surface->h * surface->pitch);
    goto cleanup;

error:
    if (size)
        *size = 0;

cleanup:
    if (surface)
        SDL_FreeSurface(surface);
}

bool save_png(const std::string& filename, const uint16_t* buffer, int width, int height) {
    SDL_Surface* surface =
        SDL_CreateRGBSurfaceWithFormatFrom((void*)buffer, width, height, SDL_BITSPERPIXEL(SDL_PIXELFORMAT_RGB565),
                                           width * SDL_BYTESPERPIXEL(SDL_PIXELFORMAT_RGB565), SDL_PIXELFORMAT_RGB565);

    int result = -1;
    if (surface) {
        result = IMG_SavePNG(surface, filename.c_str());
    }

    SDL_FreeSurface(surface);

    return result == 0;
}

void load_png_framebuffer(const std::string& filename, uint16_t* buffer) {
    load_png(filename, buffer, SDL_PIXELFORMAT_RGB565);
}

bool save_framebuffer_as_png(const std::string& filename, const uint16_t* buffer) {
    return save_png(filename, buffer, 160, 144);
}

uint16_t convert_framebuffer_pixel(uint16_t p, const uint16_t* inPalette, const uint16_t* outPalette) {
    for (uint8_t c = 0; c < 4; c++)
        if (p == inPalette[c])
            return outPalette[c];
    // checkNoEntry()
    return p;
}

void convert_framebuffer_pixels(const uint16_t* in, const uint16_t* inPalette, uint16_t* out,
                                const uint16_t* outPalette) {
    for (uint32_t i = 0; i < FRAMEBUFFER_NUM_PIXELS; i++) {
        out[i] = convert_framebuffer_pixel(in[i], inPalette, outPalette);
    }
}

bool are_framebuffer_equals(const uint16_t* buf1, const uint16_t* buf2) {
    return memcmp(buf1, buf2, FRAMEBUFFER_SIZE) == 0;
}

void dump_png_framebuffer(const std::string& filename) {
    uint16_t buffer[FRAMEBUFFER_NUM_PIXELS];
    load_png(filename, buffer, SDL_PIXELFORMAT_RGB565);
    std::cout << hexdump(buffer, FRAMEBUFFER_NUM_PIXELS) << std::endl;
}