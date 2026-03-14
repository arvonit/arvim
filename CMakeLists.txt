cmake_minimum_required(VERSION 3.25)

project(arvim
    VERSION 1.0
    DESCRIPTION "A vim-like terminal text editor"
    LANGUAGES CXX)

set(SOURCES
    src/main.cpp
    src/terminal_view.h src/terminal_view.cpp
    src/view_interface.h
    src/view_decorator.h src/view_decorator.cpp
    src/document.h src/document.cpp
    src/commands.h src/commands.cpp
    src/controller.h src/controller.cpp
    src/observer.h)

include_directories(src)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(arvim ${SOURCES})
target_compile_options(arvim PRIVATE
    -Wall -Wextra -Wpedantic -Wno-error=unused-parameter -Wno-error=unused-variable)
