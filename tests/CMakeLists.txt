set(determinismFlagsBinaryDir "${CMAKE_CURRENT_BINARY_DIR}/determinism_flags")

add_test(
    NAME determinism_flags_project
    COMMAND ${CMAKE_COMMAND} -S "${CMAKE_CURRENT_LIST_DIR}/cmake/determinism_flags" -B
            "${determinismFlagsBinaryDir}" -G "${CMAKE_GENERATOR}"
)

set_tests_properties(
    determinism_flags_project PROPERTIES FIXTURES_SETUP determinismFlagsProject LABELS slow
)

add_test(
    NAME determinism_flags
    COMMAND ${CMAKE_COMMAND} -D "UNISON_BUILD_DIR=${determinismFlagsBinaryDir}" -P
            "${CMAKE_CURRENT_LIST_DIR}/cmake/determinism_flags/verify.cmake"
)

set_tests_properties(
    determinism_flags PROPERTIES FIXTURES_REQUIRED determinismFlagsProject LABELS slow
)

add_test(
    NAME determinism_guard
    COMMAND ${CMAKE_COMMAND} -S "${CMAKE_CURRENT_LIST_DIR}/cmake/determinism_guard" -B
            "${CMAKE_CURRENT_BINARY_DIR}/determinism_guard" -G "${CMAKE_GENERATOR}"
)

set_tests_properties(determinism_guard PROPERTIES LABELS slow)

add_test(
    NAME module_determinism
    COMMAND ${CMAKE_COMMAND} -D "UNISON_BUILD_DIR=${CMAKE_BINARY_DIR}" -P
            "${CMAKE_CURRENT_LIST_DIR}/cmake/module_determinism/verify.cmake"
)

set_tests_properties(module_determinism PROPERTIES LABELS slow)

include(Catch)

add_executable(
    unison_tests_fast
    core/asset_id_test.cpp
    core/binary_io_test.cpp
    core/fixed_string_test.cpp
    core/fixed_vector_test.cpp
    core/fp_env_guard_test.cpp
    core/framework_test.cpp
    core/hasher_test.cpp
    core/log_sink_test.cpp
    core/math_test.cpp
    core/math_types_test.cpp
    core/raw_value_test.cpp
    core/rng_test.cpp
    core/xxhash_vectors_test.cpp
    net/enet_smoke_test.cpp
    sim/entt_smoke_test.cpp
    sim/jolt_smoke_test.cpp
)

target_link_libraries(unison_tests_fast PRIVATE unison_net unison_sim Catch2::Catch2WithMain)

catch_discover_tests(unison_tests_fast PROPERTIES LABELS fast)

add_library(unison_core_header_check OBJECT compile/core_headers.cpp)

target_link_libraries(unison_core_header_check PRIVATE unison_core)

unison_apply_determinism(unison_core_header_check)

unison_apply_warnings(unison_core_header_check)
