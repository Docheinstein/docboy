add_library(utils STATIC)

target_sources(utils PRIVATE
    utils/arrays.h
    utils/asserts.h
    utils/bits.h
    utils/casts.h
    utils/exceptions.h
    utils/fillqueue.h
    utils/formatters.h
    utils/hexdump.h
    utils/io.cpp
    utils/io.h
    utils/memory.cpp
    utils/memory.h
    utils/os.cpp
    utils/os.h
    utils/parcel.cpp
    utils/parcel.h
    utils/path.cpp
    utils/path.h
    utils/queue.h
    utils/random.h
    utils/std.h
    utils/strings.cpp
    utils/strings.h
    utils/traits.h
    utils/vector.h
)

target_include_directories(utils PUBLIC .)
