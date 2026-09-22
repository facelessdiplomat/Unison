add_library(unison_core STATIC src/contract.cpp src/log_sink.cpp)

target_include_directories(unison_core PUBLIC include)

target_link_libraries(unison_core PUBLIC xxhash Jolt tl::expected)

unison_apply_determinism(unison_core)

unison_apply_warnings(unison_core)
