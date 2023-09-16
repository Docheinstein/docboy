set(SOURCES
        libretro.h
        libretro.cpp
)

add_library(docboy_libretro SHARED ${SOURCES})

set_target_properties(docboy_libretro PROPERTIES PREFIX "")
target_link_libraries(docboy_libretro PRIVATE core utils)

find_program(NDK_BUILD "ndk-build")
IF (NDK_BUILD)
    message(STATUS "Found ndk-build")

    set(NDK_INTERMEDIATE ${CMAKE_BINARY_DIR}/android/intermediate)
    set(NDK_OUTPUT ${CMAKE_BINARY_DIR}/android/output)

    add_custom_target(
        docboy_libretro_android ALL
        DEPENDS docboy_libretro-android
    )

    add_custom_command(
        OUTPUT docboy_libretro-android
        COMMAND NDK_PROJECT_PATH=${CMAKE_CURRENT_SOURCE_DIR} NDK_OUT=${NDK_INTERMEDIATE} NDK_LIBS_OUT=${NDK_OUTPUT} ndk-build
    )
ENDIF()