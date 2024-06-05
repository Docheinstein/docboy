#include "extra/img/imgmanip.h"
#include "extra/png/iopng.h"

#include <cstring>

namespace {
struct ImageFormatDescription {
    uint8_t bytes_per_pixel {};
    struct {
        uint8_t r {};
        uint8_t g {};
        uint8_t b {};
    } loss {};
    struct {
        uint8_t r {};
        uint8_t g {};
        uint8_t b {};
    } shift {};
    struct {
        uint32_t r {};
        uint32_t g {};
        uint32_t b {};
    } mask {};
};

constexpr ImageFormatDescription RGB888_DESCRIPTION {3, {0, 0, 0}, {16, 8, 0}, {0xFF0000, 0x00FF00, 0x00FF}};
constexpr ImageFormatDescription RGB565_DESCRIPTION {2, {3, 2, 3}, {11, 5, 0}, {0xF800, 0x07E0, 0x001F}};

constexpr ImageFormatDescription IMAGE_FORMAT_DESCRIPTIONS[] = {RGB888_DESCRIPTION, RGB565_DESCRIPTION};

constexpr ImageFormatDescription get_image_format_description_p(ImageFormat fmt) {
    return IMAGE_FORMAT_DESCRIPTIONS[static_cast<uint8_t>(fmt)];
}

template <ImageFormat Fmt>
constexpr ImageFormatDescription get_image_format_description() {
    return IMAGE_FORMAT_DESCRIPTIONS[static_cast<uint8_t>(Fmt)];
}

uint8_t expand_pixel_byte(const uint8_t loss, const uint8_t value) {
    static constexpr uint8_t LOOKUP_0[] = {
        0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,
        22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,
        44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,
        66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,
        88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
        110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131,
        132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153,
        154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
        176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197,
        198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219,
        220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241,
        242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255};

    static constexpr uint8_t LOOKUP_1[] = {
        0,   2,   4,   6,   8,   10,  12,  14,  16,  18,  20,  22,  24,  26,  28,  30,  32,  34,  36,  38,  40,  42,
        44,  46,  48,  50,  52,  54,  56,  58,  60,  62,  64,  66,  68,  70,  72,  74,  76,  78,  80,  82,  84,  86,
        88,  90,  92,  94,  96,  98,  100, 102, 104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126, 128, 130,
        132, 134, 136, 138, 140, 142, 144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 166, 168, 170, 172, 174,
        176, 178, 180, 182, 184, 186, 188, 190, 192, 194, 196, 198, 200, 202, 204, 206, 208, 210, 212, 214, 216, 218,
        220, 222, 224, 226, 228, 230, 232, 234, 236, 238, 240, 242, 244, 246, 248, 250, 252, 255};

    static constexpr uint8_t LOOKUP_2[] = {
        0,   4,   8,   12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,  60,  64,  68,  72,  76,  80,  85,
        89,  93,  97,  101, 105, 109, 113, 117, 121, 125, 129, 133, 137, 141, 145, 149, 153, 157, 161, 165, 170, 174,
        178, 182, 186, 190, 194, 198, 202, 206, 210, 214, 218, 222, 226, 230, 234, 238, 242, 246, 250, 255};

    static constexpr uint8_t LOOKUP_3[] = {0,   8,   16,  24,  32,  41,  49,  57,  65,  74,  82,
                                           90,  98,  106, 115, 123, 131, 139, 148, 156, 164, 172,
                                           180, 189, 197, 205, 213, 222, 230, 238, 246, 255};

    static constexpr uint8_t LOOKUP_4[] = {0, 17, 34, 51, 68, 85, 102, 119, 136, 153, 170, 187, 204, 221, 238, 255};

    static constexpr uint8_t LOOKUP_5[] = {0, 36, 72, 109, 145, 182, 218, 255};

    static constexpr uint8_t LOOKUP_6[] = {0, 85, 170, 255};

    static constexpr uint8_t LOOKUP_7[] = {0, 255};

    static constexpr uint8_t LOOKUP_8[] = {255};

    static constexpr const uint8_t* LOOKUPS[] = {LOOKUP_0, LOOKUP_1, LOOKUP_2, LOOKUP_3, LOOKUP_4,
                                                 LOOKUP_5, LOOKUP_6, LOOKUP_7, LOOKUP_8};

    return LOOKUPS[loss][value];
}

uint32_t map_rgb_to_pixel_with_format(const uint8_t r, const uint8_t g, const uint8_t b,
                                      const ImageFormatDescription& fmt) {
    return (r >> fmt.loss.r) << fmt.shift.r | (g >> fmt.loss.g) << fmt.shift.g | (b >> fmt.loss.b) << fmt.shift.b;
}

void get_rgb_from_pixel_with_format(const uint32_t pixel, uint8_t& r, uint8_t& g, uint8_t& b,
                                    const ImageFormatDescription& fmt) {
    r = expand_pixel_byte(fmt.loss.r, (pixel & fmt.mask.r) >> fmt.shift.r);
    g = expand_pixel_byte(fmt.loss.g, (pixel & fmt.mask.g) >> fmt.shift.g);
    b = expand_pixel_byte(fmt.loss.b, (pixel & fmt.mask.b) >> fmt.shift.b);
}

template <ImageFormat SrcFormat>
void to_rgb888_pixel_converter(const void* src, void* dst, uint32_t index) {
    constexpr ImageFormatDescription src_fmt = get_image_format_description<SrcFormat>();
    uint32_t p_src;
    memcpy(&p_src, static_cast<const uint8_t*>(src) + index * src_fmt.bytes_per_pixel, src_fmt.bytes_per_pixel);
    uint8_t* p_dst = static_cast<uint8_t*>(dst) + index * RGB888_DESCRIPTION.bytes_per_pixel;
    get_rgb_from_pixel_with_format(p_src, *(p_dst + 0), *(p_dst + 1), *(p_dst + 2), src_fmt);
}

template <ImageFormat DstFormat, typename DstType>
void from_rgb888_pixel_converter(const void* src, void* dst, uint32_t index) {
    constexpr ImageFormatDescription dst_fmt = get_image_format_description<DstFormat>();
    const uint8_t* p_src = static_cast<const uint8_t*>(src) + index * RGB888_DESCRIPTION.bytes_per_pixel;
    DstType* p_dst = static_cast<DstType*>(dst) + index;
    *p_dst = map_rgb_to_pixel_with_format(p_src[0], p_src[1], p_src[2], dst_fmt);
}

template <ImageFormat DstFormat>
void from_rgb888_pixel_converter(const void* src, void* dst, uint32_t index) {
    constexpr ImageFormatDescription dst_fmt = get_image_format_description<DstFormat>();
    if constexpr (dst_fmt.bytes_per_pixel == 1) {
        from_rgb888_pixel_converter<DstFormat, uint8_t>(src, dst, index);
    } else if constexpr (dst_fmt.bytes_per_pixel == 2) {
        from_rgb888_pixel_converter<DstFormat, uint16_t>(src, dst, index);
    } else if constexpr (dst_fmt.bytes_per_pixel == 4) {
        from_rgb888_pixel_converter<DstFormat, uint32_t>(src, dst, index);
    } else {
        static_assert(static_cast<uint32_t>(DstFormat) == UINT32_MAX); // always false
    }
}

template <ImageFormat Format>
void identity_pixel_converter(const void* src, void* dst, uint32_t index) {
    constexpr ImageFormatDescription fmt = get_image_format_description<Format>();
    const uint8_t* p_src = static_cast<const uint8_t*>(src) + index * fmt.bytes_per_pixel;
    uint8_t* p_dst = static_cast<uint8_t*>(dst) + index * fmt.bytes_per_pixel;
    memcpy(p_dst, p_src, fmt.bytes_per_pixel);
}

using pixel_converter = void (*)(const void* /* src */, void* /* dst */, uint32_t /* index */);

pixel_converter create_pixel_converter(ImageFormat from, ImageFormat to) {
    if (from == ImageFormat::RGB888) {
        if (to == ImageFormat::RGB565) {
            return from_rgb888_pixel_converter<ImageFormat::RGB565>;
        }
    }
    if (to == ImageFormat::RGB888) {
        if (from == ImageFormat::RGB565) {
            return to_rgb888_pixel_converter<ImageFormat::RGB565>;
        }
    }
    if (from == to) {
        if (from == ImageFormat::RGB888) {
            return identity_pixel_converter<ImageFormat::RGB888>;
        }
        if (from == ImageFormat::RGB565) {
            return identity_pixel_converter<ImageFormat::RGB565>;
        }
    }
    abort();
}
} // namespace

void convert_image(ImageFormat from, const void* src, ImageFormat to, void* dst, uint32_t width, uint32_t height) {
    const pixel_converter converter = create_pixel_converter(from, to);
    for (uint32_t i = 0; i < width * height; i++) {
        converter(src, dst, i);
    }
}

std::vector<uint8_t> create_image_buffer(uint32_t width, uint32_t height, ImageFormat fmt) {
    std::vector<uint8_t> buf {};
    buf.resize(width * height * get_image_format_description_p(fmt).bytes_per_pixel);
    return buf;
}

uint32_t rgb_to_pixel(uint8_t r, uint8_t g, uint8_t b, ImageFormat fmt) {
    return map_rgb_to_pixel_with_format(r, g, b, get_image_format_description_p(fmt));
}

void pixel_to_rgb(uint32_t pixel, uint8_t& r, uint8_t& g, uint8_t& b, ImageFormat fmt) {
    return get_rgb_from_pixel_with_format(pixel, r, g, b, get_image_format_description_p(fmt));
}

uint32_t convert_pixel(uint32_t pixel, ImageFormat from, ImageFormat to) {
    uint8_t r, g, b;
    pixel_to_rgb(pixel, r, g, b, from);
    return rgb_to_pixel(r, g, b, to);
}
