set(SOURCES
        termcolor.h
)

add_library(termcolor STATIC ${SOURCES})
set_target_properties(termcolor PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(termcolor PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
