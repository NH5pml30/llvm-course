add_library(app app.c)

if (INSTR_USE_PASS)
  set(APP_INSTR_PASS InstrUseTracePass)
else()
  set(APP_INSTR_PASS InstrTracePass)
endif()

add_custom_target(pass DEPENDS ${APP_INSTR_PASS})
add_dependencies(app pass)

target_compile_options(app PRIVATE "-fpass-plugin=$<TARGET_FILE:${APP_INSTR_PASS}>")
