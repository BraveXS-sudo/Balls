cmake_minimum_required(VERSION 3.10)
project(balls)

set(CMAKE_C_STANDARD 99)

find_package(raylib REQUIRED)

add_executable(balls 
    src/main.c 
    src/steam.c 
    src/ui.c 
    src/storage.c 
    src/desktop_entry.c
    src/categories.c
    src/overrides.c
    src/utils.c
)

target_link_libraries(balls raylib m)
