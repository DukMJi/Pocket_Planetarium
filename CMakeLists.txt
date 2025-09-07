cmake_minimum_required(VERSION 3.16)
project(pocket_planetarium C)
set(CMAKE_C_STANDARD 11)

# Tell CMake to also process the firmware folder
add_subdirectory(firmware)
