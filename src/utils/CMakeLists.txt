add_library(utils STATIC)

target_sources(utils PRIVATE
    utils/arrays.h
    utils/asserts.h
    utils/bits.hpp
    utils/casts.hpp
    utils/chrono.hpp
    utils/exceptions.hpp
    utils/fillqueue.hpp
    utils/formatters.hpp
    utils/hexdump.hpp
    utils/io.cpp
    utils/io.h
    utils/log.h
    utils/macros.h
    utils/mathematics.h
    utils/memory.cpp
    utils/memory.h
    utils/os.cpp
    utils/os.h
    utils/parcel.cpp
    utils/parcel.h
    utils/path.cpp
    utils/path.h
    utils/queue.hpp
    utils/random.hpp
    utils/std.hpp
    utils/strings.cpp
    utils/strings.hpp
    utils/traits.h
    utils/vector.hpp
)

target_include_directories(utils PUBLIC .)
