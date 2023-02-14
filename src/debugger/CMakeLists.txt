set(SOURCES
        debuggerbackend.h
        debuggerbackend.cpp
        debuggerfrontend.h
        debuggerfrontend.cpp
        debuggerfrontendcli.h
        debuggerfrontendcli.cpp
        instructions.h
        instructions.cpp
)

add_library(debugger STATIC ${SOURCES})
target_link_libraries(debugger PRIVATE termcolor utils)
target_include_directories(debugger PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
