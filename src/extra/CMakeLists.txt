add_library(extra STATIC EXCLUDE_FROM_ALL)

target_sources(extra PRIVATE
    extra/cartridge/header.h
    extra/cartridge/header.cpp
    extra/ini/reader/reader.h
    extra/ini/reader/reader.tpp
    extra/ini/reader/reader.cpp
    extra/ini/writer/writer.cpp
    extra/ini/writer/writer.h
    extra/png/iopng.h
    extra/png/iopng.cpp
    extra/img/imgmanip.h
    extra/img/imgmanip.cpp
)

if (ENABLE_SERIAL)
    target_sources(extra PRIVATE
        extra/serial/endpoints/buffer.cpp
        extra/serial/endpoints/buffer.h
        extra/serial/endpoints/console.cpp
        extra/serial/endpoints/console.h
    )
endif ()

target_link_libraries(extra PRIVATE utils docboy png_static)

target_include_directories(extra PUBLIC .)
