find_package(SDL2 REQUIRED)

add_library(sim sim.c)
target_link_libraries(sim PRIVATE ${SDL2_LIBRARIES})
target_include_directories(sim PRIVATE ${SDL2_INCLUDE_DIRS})
target_compile_options(sim PRIVATE ${SDL2_CFLAGS_OTHER})
