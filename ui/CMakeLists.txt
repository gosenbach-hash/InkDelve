cmake_minimum_required(VERSION 3.5)
project(inkdelve CXX)

add_executable(inkdelve.app main.cpp gfx.cpp ../core/grid.cpp ../core/game.cpp)
target_include_directories(inkdelve.app PRIVATE ../core)
target_link_libraries(inkdelve.app inkview)
