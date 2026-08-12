cmake_minimum_required(VERSION 3.14)
project(chip8 C)

set(CMAKE_C_STANDARD 99)

include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        6.0
)
FetchContent_MakeAvailable(raylib)

add_executable(${PROJECT_NAME} main.c)

target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
