cmake_minimum_required(VERSION 3.16)

find_package(SDL2 REQUIRED CONFIG)
find_package(SDL2_ttf REQUIRED CONFIG)

add_executable(pocket_planetarium src/main.c)
target_link_libraries(pocket_planetarium PRIVATE SDL2::SDL2 SDL2_ttf::SDL2_ttf)
