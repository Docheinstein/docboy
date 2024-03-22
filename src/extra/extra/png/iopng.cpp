#include "iopng.h"

#include <cstring>
#include <iostream>

#include "png.h"

// https://gist.github.com/niw/5963798

bool save_png_rgb888(const std::string& filename, const void* buffer, uint32_t width, uint32_t height) {
    FILE* fp {};
    png_structp png {};
    png_infop info {};

    png_byte** rowPointers {};

    bool success {};

    png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png)
        goto error;

    info = png_create_info_struct(png);
    if (!info)
        goto error;

    if (setjmp(png_jmpbuf(png)))
        goto error;

    fp = fopen(filename.c_str(), "wb");
    if (!fp)
        goto error;

    png_init_io(png, fp);

    png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    rowPointers = new png_byte*[height];

    for (uint32_t y = 0; y < height; y++) {
        rowPointers[y] = const_cast<unsigned char*>(static_cast<const unsigned char*>(buffer)) + y * width * 3;
    }

    png_write_image(png, rowPointers);
    png_write_end(png, nullptr);

    success = true;

error:
    if (fp)
        fclose(fp);

    png_destroy_write_struct(&png, &info);

    delete[] rowPointers;

    return success;
}

std::vector<uint8_t> load_png_rgb888(const std::string& filename, uint32_t* width_out, uint32_t* height_out, bool* ok) {
    FILE* fp {};
    png_structp png {};
    png_infop info {};

    png_byte** rowPointers {};

    uint32_t width {};
    uint32_t height {};
    png_byte color_type {};
    png_byte bit_depth {};

    uint32_t bytesPerRow {};

    bool success {};

    std::vector<uint8_t> result;

    fp = fopen(filename.c_str(), "rb");
    if (!fp)
        goto error;

    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png)
        goto error;

    info = png_create_info_struct(png);
    if (!info)
        goto error;

    if (setjmp(png_jmpbuf(png)))
        goto error;

    png_init_io(png, fp);

    png_read_info(png, info);

    width = png_get_image_width(png, info);
    height = png_get_image_height(png, info);
    color_type = png_get_color_type(png, info);
    bit_depth = png_get_bit_depth(png, info);

    if (bit_depth == 16)
        png_set_strip_16(png);

    switch (color_type) {
    case PNG_COLOR_TYPE_PALETTE:
        png_set_palette_to_rgb(png);
        break;
    case PNG_COLOR_TYPE_GRAY:
    case PNG_COLOR_TYPE_GRAY_ALPHA:
        png_set_gray_to_rgb(png);
    case PNG_COLOR_TYPE_RGB_ALPHA:
        png_set_strip_alpha(png);
    }

    png_read_update_info(png, info);

    rowPointers = new png_byte*[height];

    bytesPerRow = png_get_rowbytes(png, info);

    for (uint32_t y = 0; y < height; y++) {
        rowPointers[y] = new png_byte[bytesPerRow];
    }

    png_read_image(png, rowPointers);

    result.resize(height * bytesPerRow);

    for (uint32_t y = 0; y < height; y++) {
        memcpy(result.data() + y * bytesPerRow, rowPointers[y], bytesPerRow);
    }

    success = true;

    if (width_out)
        *width_out = width;

    if (height_out)
        *height_out = height;

error:
    if (fp)
        fclose(fp);

    png_destroy_read_struct(&png, &info, nullptr);

    if (rowPointers) {
        for (uint32_t y = 0; y < height; y++) {
            delete[] rowPointers[y];
        }
        delete[] rowPointers;
    }

    if (ok)
        *ok = success;

    return result;
}
