cmake_minimum_required(VERSION 3.5.0)
project(3 VERSION 0.1.0 LANGUAGES C CXX)

add_subdirectory(sim)

find_package(LLVM REQUIRED CONFIG)
include(AddLLVM)

include_directories(${LLVM_INCLUDE_DIRS} sim)
add_definitions(${LLVM_DEFINITIONS})

add_executable(${PROJECT_NAME} app_ir_gen.cpp)
target_link_libraries(${PROJECT_NAME} sim LLVM)
