if (BUILD_LIBRETRO_CORE_ANDROID)
    find_program(NDK_BUILD_EXECUTABLE "ndk-build" PATHS ${NDK_PATH} REQUIRED)
    set(NDK_INTERMEDIATE ${CMAKE_BINARY_DIR}/android/intermediate)
    set(NDK_OUTPUT ${CMAKE_BINARY_DIR}/android/output)

    add_custom_target(
        docboy_libretro_android ALL
        DEPENDS docboy_libretro-android
    )

    add_custom_command(
        OUTPUT docboy_libretro-android
        COMMAND NDK_PROJECT_PATH=${CMAKE_CURRENT_SOURCE_DIR} NDK_OUT=${NDK_INTERMEDIATE} NDK_LIBS_OUT=${NDK_OUTPUT} ${NDK_BUILD_EXECUTABLE}
    )
endif ()