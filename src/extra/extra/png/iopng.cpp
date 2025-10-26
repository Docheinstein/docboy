#include "extra/png/iopng.h"

#include <cstring>
#include <iostream>

#include "png.h"

// https://gist.github.com/niw/5963798

bool save_png_rgb888(const std::string& filename, const void* buffer, uint32_t width, uint32_t height) {
    FILE* fp {};
    png_structp png {};
    png_infop info {};

    png_byte** raw_pointers {};

    bool success {};

    png = docboy_png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        goto error;
    }

    info = docboy_png_create_info_struct(png);
    if (!info) {
        goto error;
    }

    if (setjmp(png_jmpbuf(png))) {
        goto error;
    }

    fp = fopen(filename.c_str(), "wb");
    if (!fp) {
        goto error;
    }

    docboy_png_init_io(png, fp);

    docboy_png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                        PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    docboy_png_write_info(png, info);

    raw_pointers = new png_byte*[height];

    for (uint32_t y = 0; y < height; y++) {
        raw_pointers[y] = const_cast<unsigned char*>(static_cast<const unsigned char*>(buffer)) + y * width * 3;
    }

    docboy_png_write_image(png, raw_pointers);
    docboy_png_write_end(png, nullptr);

    success = true;

error:
    if (fp) {
        fclose(fp);
    }

    docboy_png_destroy_write_struct(&png, &info);

    delete[] raw_pointers;

    return success;
}

std::vector<uint8_t> load_png_rgb888(const std::string& filename, uint32_t* width_out, uint32_t* height_out, bool* ok) {
    FILE* fp {};
    png_structp png {};
    png_infop info {};

    png_byte** raw_pointers {};

    uint32_t width {};
    uint32_t height {};
    png_byte color_type {};
    png_byte bit_depth {};

    uint32_t bytes_per_row {};

    bool success {};

    std::vector<uint8_t> result;

    fp = fopen(filename.c_str(), "rb");
    if (!fp) {
        goto error;
    }

    png = docboy_png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        goto error;
    }

    info = docboy_png_create_info_struct(png);
    if (!info) {
        goto error;
    }

    if (setjmp(png_jmpbuf(png))) {
        goto error;
    }

    docboy_png_init_io(png, fp);

    docboy_png_read_info(png, info);

    width = docboy_png_get_image_width(png, info);
    height = docboy_png_get_image_height(png, info);
    color_type = docboy_png_get_color_type(png, info);
    bit_depth = docboy_png_get_bit_depth(png, info);

    if (bit_depth == 16) {
        docboy_png_set_strip_16(png);
    }

    switch (color_type) {
    case PNG_COLOR_TYPE_PALETTE:
        docboy_png_set_palette_to_rgb(png);
        break;
    case PNG_COLOR_TYPE_GRAY:
    case PNG_COLOR_TYPE_GRAY_ALPHA:
        docboy_png_set_gray_to_rgb(png);
    case PNG_COLOR_TYPE_RGB_ALPHA:
        docboy_png_set_strip_alpha(png);
    }

    docboy_png_read_update_info(png, info);

    raw_pointers = new png_byte*[height];

    bytes_per_row = docboy_png_get_rowbytes(png, info);

    for (uint32_t y = 0; y < height; y++) {
        raw_pointers[y] = new png_byte[bytes_per_row];
    }

    docboy_png_read_image(png, raw_pointers);

    result.resize(height * bytes_per_row);

    for (uint32_t y = 0; y < height; y++) {
        memcpy(result.data() + y * bytes_per_row, raw_pointers[y], bytes_per_row);
    }

    success = true;

    if (width_out) {
        *width_out = width;
    }

    if (height_out) {
        *height_out = height;
    }
error:
    if (fp) {
        fclose(fp);
    }

    docboy_png_destroy_read_struct(&png, &info, nullptr);

    if (raw_pointers) {
        for (uint32_t y = 0; y < height; y++) {
            delete[] raw_pointers[y];
        }
        delete[] raw_pointers;
    }

    if (ok) {
        *ok = success;
    }

    return result;
}
