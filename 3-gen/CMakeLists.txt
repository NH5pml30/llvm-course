cmake_minimum_required(VERSION 3.7)

project("three")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 23)

find_package(antlr4-runtime REQUIRED)
find_package(antlr4-generator REQUIRED)

set(ANTLR4_JAR_LOCATION /usr/share/java/antlr-complete.jar)

antlr4_generate(
    three_antlr
    ${CMAKE_CURRENT_SOURCE_DIR}/IR.g4
    BOTH
    FALSE
    TRUE
    "three"
)

include_directories(${PROJECT_BINARY_DIR} ${ANTLR4_INCLUDE_DIR} ${ANTLR4_INCLUDE_DIR_three_antlr})

add_executable(three main.cpp ${ANTLR4_SRC_FILES_three_antlr})

add_dependencies(three antlr4_shared)

target_link_libraries(three PRIVATE antlr4_shared)