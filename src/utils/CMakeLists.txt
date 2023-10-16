add_library(utils STATIC)

target_sources(utils PRIVATE
    arrays.h
    asserts.h
    bits.hpp
    casts.hpp
    chrono.hpp
    exceptions.hpp
    fillqueue.hpp
    formatters.hpp
    hexdump.hpp
    io.cpp
    io.h
    log.h
    macros.h
    math.h
    memory.h
    memory.cpp
    os.cpp
    os.h
    parcel.cpp
    parcel.h
    path.cpp
    path.h
    queue.hpp
    std.hpp
    strings.cpp
    strings.hpp
    termcolor.hpp
    vector.hpp
)
target_include_directories(utils PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
