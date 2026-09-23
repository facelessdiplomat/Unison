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
    COMMAND
        ${CMAKE_COMMAND} -D "UNISON_BUILD_DIR=${CMAKE_BINARY_DIR}" -D
        "UNISON_INSTRUCTION_SET=${UNISON_INSTRUCTION_SET}" -P
        "${CMAKE_CURRENT_LIST_DIR}/cmake/module_determinism/verify.cmake"
)

set_tests_properties(module_determinism PROPERTIES LABELS slow)

include(Catch)

add_executable(
    unison_tests_fast
    core/asset_id_test.cpp
    core/binary_io_test.cpp
    core/body_id_test.cpp
    core/contract_test.cpp
    core/error_test.cpp
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
    session/input_buffer_test.cpp
    session/local_input_test.cpp
    session/repeat_last_input_predictor_test.cpp
    session/session_config_test.cpp
    session/session_rollback_test.cpp
    session/session_test.cpp
    session/snapshot_ring_test.cpp
    sim/advance_frame_test.cpp
    sim/asset_hash_test.cpp
    sim/asset_registry_test.cpp
    sim/body_id_allocator_test.cpp
    sim/character_test.cpp
    sim/component_registry_test.cpp
    sim/entt_smoke_test.cpp
    sim/entity_lifecycle_test.cpp
    sim/event_buffer_test.cpp
    sim/frame_checksum_test.cpp
    sim/frame_inputs_test.cpp
    sim/frame_snapshot_test.cpp
    sim/frame_test.cpp
    sim/jolt_layers_test.cpp
    sim/jolt_runtime_test.cpp
    sim/jolt_smoke_test.cpp
    sim/padding_free_test.cpp
    sim/physics_body_test.cpp
    sim/physics_contact_test.cpp
    sim/physics_determinism_test.cpp
    sim/physics_layer_test.cpp
    sim/physics_query_test.cpp
    sim/physics_snapshot_test.cpp
    sim/physics_step_test.cpp
    sim/physics_world_test.cpp
    sim/pipeline_hash_test.cpp
    sim/registry_clone_test.cpp
    sim/signals_test.cpp
    sim/system_pipeline_test.cpp
    support/test_components.cpp
)

target_include_directories(unison_tests_fast PRIVATE "${CMAKE_CURRENT_LIST_DIR}")

target_link_libraries(unison_tests_fast PRIVATE unison_net unison_session unison_sim Catch2::Catch2WithMain)

target_compile_options(unison_tests_fast PRIVATE /EHsc)

catch_discover_tests(unison_tests_fast PROPERTIES LABELS fast)

add_library(unison_core_header_check OBJECT compile/core_headers.cpp)

target_link_libraries(unison_core_header_check PRIVATE unison_core)

unison_apply_determinism(unison_core_header_check)

unison_apply_warnings(unison_core_header_check)

add_executable(
    unison_tests_arena arena/apply_input_test.cpp arena/arena_input_test.cpp
    arena/arena_simulation_test.cpp arena/assets_test.cpp
    arena/character_move_test.cpp arena/components_test.cpp arena/crates_test.cpp arena/determinism_test.cpp arena/golden_test.cpp
    arena/hits_test.cpp arena/lifetimes_test.cpp
    arena/match_rules_test.cpp arena/respawn_test.cpp arena/rollback_test.cpp arena/session_test.cpp arena/shot_path_test.cpp
    arena/snapshot_room_test.cpp arena/weapons_test.cpp
)

target_include_directories(unison_tests_arena PRIVATE "${CMAKE_CURRENT_LIST_DIR}")

target_link_libraries(unison_tests_arena PRIVATE arena_sim unison_session Catch2::Catch2WithMain)

target_compile_options(unison_tests_arena PRIVATE /EHsc)

target_compile_definitions(
    unison_tests_arena PRIVATE "UNISON_GOLDEN_DIR=\"${CMAKE_CURRENT_LIST_DIR}/golden\""
)

catch_discover_tests(unison_tests_arena PROPERTIES LABELS fast)

add_executable(unison_benchmarks benchmarks/arena_benchmarks.cpp)

target_include_directories(unison_benchmarks PRIVATE "${CMAKE_CURRENT_LIST_DIR}")

target_link_libraries(unison_benchmarks PRIVATE arena_sim Catch2::Catch2WithMain)

target_compile_options(unison_benchmarks PRIVATE /EHsc)
