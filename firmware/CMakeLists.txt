cmake_minimum_required(VERSION 3.16)

find_package(SDL2 REQUIRED CONFIG)
find_package(SDL2_ttf REQUIRED CONFIG)

add_executable(pocket_planetarium
    src/main.c
    src/imu.c
)

target_include_directories(pocket_planetarium PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pocket_planetarium PRIVATE
    SDL2::SDL2
    SDL2_ttf::SDL2_ttf
)