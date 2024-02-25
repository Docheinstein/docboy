add_library(extra STATIC EXCLUDE_FROM_ALL)

target_sources(extra PRIVATE
    extra/cartridge/header.h
    extra/cartridge/header.cpp
    extra/config/parser.h
    extra/config/parser.tpp
    extra/config/parser.cpp
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
