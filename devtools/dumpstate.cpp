#include <cstring>

#include <iostream>

#include "testutils/framebuffers.h"
#include "utils/hexdump.h"

#include "utils/io.h"
#include "utils/strings.h"

// Script for dump the content of a .state file in a human-readable format.
// Note: the state file should be produced by a docboy compiled with both
// -DENABLE_STATE_DEBUG_SYMBOLS=ON and -DENABLE_ASSERTS=ON.

// usage: dumpstate <in_file> [--hide-bytes]

enum ParcelDataTypes : uint8_t {
    Bool = 0,
    Uint8 = 1,
    Uint16 = 2,
    Uint32 = 3,
    Uint64 = 4,
    Int8 = 5,
    Int16 = 6,
    Int32 = 7,
    Int64 = 8,
    Double = 9,
    String = 10,
    Bytes = 11,
    LastType = Bytes
};

std::string parcel_data_type_to_string(uint8_t type) {
    switch (type) {
    case Bool:
        return "bool";
    case Uint8:
        return "uint8";
    case Uint16:
        return "uint16";
    case Uint32:
        return "uint32";
    case Uint64:
        return "uint64";
    case Int8:
        return "int8";
    case Int16:
        return "int16";
    case Int32:
        return "int32";
    case Int64:
        return "int64";
    case Double:
        return "double";
    case String:
        return "string";
    case Bytes:
        return "bytes";
    default:
        std::cerr << "Unexpected data type '" << type << "'" << std::endl;
        exit(1);
    }
}

uint32_t hash_of(const std::vector<uint8_t>& data) {
    uint64_t hash = 17;
    for (const uint8_t u : data) {
        hash = hash * 31 + u;
    }
    return hash;
}

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: dumpstate <in_file> [--hide-bytes]" << std::endl;
        exit(1);
    }

    bool hide_bytes = argc > 2 && strcmp(argv[2], "--hide-bytes") == 0;

    bool ok;
    auto content = read_file(argv[1], &ok);

    if (!ok) {
        std::cerr << "Failed to read file '" << argv[1] << "'" << std::endl;
        return 1;
    }

    const uint8_t* data = content.data();

    uint32_t cursor = 0;

    auto read_data = [data, &cursor](void* dst, uint32_t count) {
        memcpy(dst, data + cursor, count);
        cursor += count;
    };

    while (cursor < content.size()) {
        // We expect each serialized field having the following information:
        // - Label type (always Types.String)
        // - Label length (uint32)
        // - Label (C-string)
        // - Data type (one of Types)
        // - Data
        uint8_t label_type;
        read_data(&label_type, sizeof(label_type));

        if (label_type != String) {
            std::cerr << "Unexpected data type. State must be saved with -DENABLE_STATE_DEBUG_SYMBOLS=ON and "
                         "-DENABLE_ASSERTS=ON"
                      << std::endl;
            return 1;
        }

        uint32_t label_size;
        read_data(&label_size, sizeof(label_size));

        std::string label;
        label.resize(label_size + 1);
        label[label_size] = '\0';
        read_data(label.data(), label_size);

        uint8_t data_type;
        read_data(&data_type, sizeof(data_type));
        if (data_type > LastType) {
            std::cerr << "Unexpected data type '" << data_type << "'" << std::endl;
            return 1;
        }

        std::cout << "[" << rpad(parcel_data_type_to_string(data_type), 7) << "] " << label << " = ";
        switch (data_type) {
        case Bool: {
            bool b;
            read_data(&b, sizeof(b));
            std::cout << (b ? "true" : "false") << std::endl;
            break;
        }
        case Uint8: {
            uint8_t u;
            read_data(&u, sizeof(u));
            std::cout << +u << std::endl;
            break;
        }
        case Uint16: {
            uint16_t u;
            read_data(&u, sizeof(u));
            std::cout << u << std::endl;
            break;
        }
        case Uint32: {
            uint32_t u;
            read_data(&u, sizeof(u));
            std::cout << u << std::endl;
            break;
        }
        case Uint64: {
            uint64_t u;
            read_data(&u, sizeof(u));
            std::cout << u << std::endl;
            break;
        }
        case Int8: {
            int8_t i;
            read_data(&i, sizeof(i));
            std::cout << +i << std::endl;
            break;
        }
        case Int16: {
            int16_t i;
            read_data(&i, sizeof(i));
            std::cout << i << std::endl;
            break;
        }
        case Int32: {
            int32_t i;
            read_data(&i, sizeof(i));
            std::cout << i << std::endl;
            break;
        }
        case Int64: {
            int64_t i;
            read_data(&i, sizeof(i));
            std::cout << i << std::endl;
            break;
        }
        case Double: {
            double d;
            read_data(&d, sizeof(d));
            std::cout << d << std::endl;
            break;
        }
        case String: {
            uint32_t len;
            read_data(&len, sizeof(len));
            std::string s;
            s.resize(len);
            s[len] = '\0';
            read_data(s.data(), len);
            std::cout << s << std::endl;
            break;
        }
        case Bytes: {
            uint32_t len;
            read_data(&len, sizeof(len));
            std::vector<uint8_t> b;
            b.resize(len);
            read_data(b.data(), len);
            std::cout << "(" << len << " bytes, hash=" << hash_of(b) << ")" << std::endl;
            if (!hide_bytes) {
                std::cout << hexdump(b.data(), len) << std::endl;
            }
            break;
        }
        default: {
            std::cerr << "Unexpected data type '" << data_type << "'" << std::endl;
            return 1;
        }
        }
    }

    return 0;
}