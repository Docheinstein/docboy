template<uint8_t n, typename T>
uint8_t get_byte(T value) {
    return value >> (8 * n);
}

template<uint8_t n, typename T>
void set_byte(T &dest, uint8_t value) {
    dest &= ((uint8_t) 0) << (8 * n);
    dest |= (value << (8 * n));
}


template<uint8_t n, typename T>
bool get_bit(T value) {
    return (value >> n) & 1;
}

template<uint8_t n, typename T>
void set_bit(T &dest, bool value) {
    dest &= (~((uint8_t) 1 << n));
    dest |= ((value ? 1 : 0) << n);
}

template<typename T>
std::string bin(T value) {
    std::stringstream ss;
    bin(value, ss);
    return ss.str();
}

template<typename T>
std::string bin(const T *data, size_t length) {
    std::stringstream ss;

    for (size_t i = 0; i < length; i++) {
        bin(data[i], ss);
        if (i < length - 1)
            ss << " ";
    }
    return ss.str();
}

template<typename T>
std::string hex(T value) {
    std::stringstream ss;
    hex(value, ss);
    return ss.str();
}

template<typename T>
std::string hex(const T *data, size_t length) {
    std::stringstream ss;

    for (size_t i = 0; i < length; i++) {
        hex(data[i], ss);
        if (i < length - 1)
            ss << " ";
    }
    return ss.str();
}

template<typename T>
std::string hexdump(const T *data, size_t length, int columns) {
    std::stringstream ss;
    for (size_t i = 0; i < length; i++) {
        if (i % columns == 0)
            ss << std::hex << std::setfill('0') << std::setw(8) << i << " | ";
        hex(data[i], ss);
        if ((i + 1) % columns == 0) {
            if ((i + 1) < length)
                ss << std::endl;
        } else {
            ss << " ";
            if ((i + 1) % 8 == 0)
                ss << " ";
        }
    }
    return ss.str();
}

template<typename T>
std::string hexdump(const std::vector<T> &vec) {
    return hexdump(vec.data(), vec.size() * sizeof(T));
}
