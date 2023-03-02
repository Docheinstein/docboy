set(SOURCES
    cartridge/cartridge.h
    cartridge/cartridge.cpp
    cartridge/cartridgefactory.h
    cartridge/cartridgefactory.cpp
    cartridge/mbc1.h
    cartridge/mbc1.cpp
    cartridge/mbc1ram.h
    cartridge/mbc1ram.cpp
    cartridge/nombc.h
    cartridge/nombc.cpp
    serial/serial.h
    serial/serial.cpp
    serial/serialbuffer.h
    serial/serialbuffer.cpp
    serial/serialconsole.h
    serial/serialconsole.cpp
    bus.h
    bus.cpp
    core.h
    core.cpp
    cpu.h
    cpu.cpp
    definitions.h
    gameboy.h
    gameboy.cpp
    helpers.h
    helpers.cpp
    io.h
    io.cpp

    memory.h
    memory.cpp

)

add_library(core STATIC ${SOURCES})

if (ENABLE_DEBUGGER)
    target_sources(core PRIVATE
        debugger/debuggablebus.h
        debugger/debuggablebus.cpp
        debugger/debuggablecore.h
        debugger/debuggablecore.cpp
        debugger/debuggablecpu.h
        debugger/debuggablecpu.cpp
        debugger/debuggerbackend.h
        debugger/debuggerbackend.cpp
        debugger/debuggerfrontend.h
        debugger/debuggerfrontend.cpp
        debugger/debuggerfrontendcli.h
        debugger/debuggerfrontendcli.cpp
    )
    target_compile_definitions(core PUBLIC ENABLE_DEBUGGER)
endif()

target_include_directories(core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
