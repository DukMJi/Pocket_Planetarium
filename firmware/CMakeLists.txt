cmake_minimum_required(VERSION 3.16)

# Use SDL2's CMake package installed by Homebrew
find_package(SDL2 REQUIRED CONFIG)

add_executable(pocket_planetarium src/main.c)
target_link_libraries(pocket_planetarium PRIVATE SDL2::SDL2)
