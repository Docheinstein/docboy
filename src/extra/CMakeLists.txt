add_library(extra STATIC EXCLUDE_FROM_ALL)

target_sources(extra PRIVATE
    cartridge/header.h
    cartridge/header.cpp
)

if (ENABLE_SERIAL)
    target_sources(extra PRIVATE
        serial/endpoints/buffer.cpp
        serial/endpoints/buffer.h
        serial/endpoints/console.cpp
        serial/endpoints/console.h
    )
endif ()

target_link_libraries(extra PRIVATE utils docboy)
