cmake_minimum_required(VERSION 3.5.0)
project(1 VERSION 0.1.0 LANGUAGES C)

find_package(SDL2 REQUIRED)

add_library(sim sim.c)
target_link_libraries(sim PRIVATE ${SDL2_LIBRARIES})
target_include_directories(sim PRIVATE ${SDL2_INCLUDE_DIRS})
target_compile_options(sim PRIVATE ${SDL2_CFLAGS_OTHER})

add_library(app app.c)

add_executable(${PROJECT_NAME} start.c)
target_link_libraries(${PROJECT_NAME} app sim)

add_custom_target(IR
  COMMAND ${CMAKE_C_COMPILER} -Oz -S -emit-llvm ${CMAKE_CURRENT_SOURCE_DIR}/app.c -o app.ll
  DEPENDS app.c)

add_dependencies(${PROJECT_NAME} IR)
