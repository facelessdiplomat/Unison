add_library(unison_core STATIC src/core.cpp)

target_include_directories(unison_core PUBLIC include)

unison_apply_determinism(unison_core)
