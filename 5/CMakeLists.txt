cmake_minimum_required(VERSION 3.7)

project("five")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 20)

find_package(antlr4-runtime REQUIRED)
find_package(antlr4-generator REQUIRED)

# set(ANTLR4_JAR_LOCATION /usr/share/java/antlr-complete.jar)
set(ANTLR4_JAR_LOCATION /home/nh5/antlr-4.13.2-complete.jar)

antlr4_generate(
    five_antlr
    ${CMAKE_CURRENT_SOURCE_DIR}/HA.g4
    BOTH
    TRUE
    TRUE
    "five"
)

find_package(LLVM REQUIRED CONFIG)

add_executable(five main.cpp ${ANTLR4_SRC_FILES_five_antlr})
target_include_directories(five PRIVATE ${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR} ${ANTLR4_INCLUDE_DIR} ${ANTLR4_INCLUDE_DIR_five_antlr} ${LLVM_INCLUDE_DIRS})
target_compile_definitions(five PRIVATE ${LLVM_DEFINITIONS})
add_dependencies(five antlr4_shared)

find_package(SDL2 REQUIRED)
add_library(sim sim/sim.c)
target_link_libraries(sim PRIVATE ${SDL2_LIBRARIES})
target_include_directories(sim PRIVATE ${SDL2_INCLUDE_DIRS})
target_compile_options(sim PRIVATE ${SDL2_CFLAGS_OTHER})

target_link_libraries(five PRIVATE sim antlr4_shared LLVM)
