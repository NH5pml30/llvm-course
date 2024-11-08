cmake_minimum_required(VERSION 3.5.0)
project(4 VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 23)

find_package(SDL2 REQUIRED)

add_library(sim sim.c)
target_link_libraries(sim PRIVATE ${SDL2_LIBRARIES})
target_include_directories(sim PRIVATE ${SDL2_INCLUDE_DIRS})
target_compile_options(sim PRIVATE ${SDL2_CFLAGS_OTHER})

add_executable(4_sim sim/cpu.cpp sim/asm.cpp sim/main_sim.cpp)
target_link_libraries(4_sim sim)

add_executable(4_func func/main_func.cpp)
# target_link_libraries(4_func sim)
