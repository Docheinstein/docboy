add_library(tui STATIC)

target_sources(tui PUBLIC
    block.cpp
    block.h
    container.cpp
    container.h
    decorator.h
    decorators.cpp
    decorators.h
    decorators.tpp
    divider.h
    factory.cpp
    factory.h
    hlayout.h
    node.h
    presenter.cpp
    presenter.h
    text.cpp
    text.h
    token.h
    traits.h
    vlayout.h
)

target_include_directories(tui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

