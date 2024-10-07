cmake_minimum_required(VERSION 3.5.0)
project(2 VERSION 0.1.0 LANGUAGES C CXX)

add_subdirectory(pass)
add_subdirectory(app)
add_subdirectory(sim)

add_executable(${PROJECT_NAME} start.c)
target_link_libraries(${PROJECT_NAME} app sim)
