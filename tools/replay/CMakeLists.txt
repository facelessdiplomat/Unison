add_library(unison_replay_core STATIC src/replay_options.cpp src/replay_report.cpp)

target_include_directories(unison_replay_core PUBLIC include)

target_link_libraries(unison_replay_core PUBLIC unison_session PRIVATE cxxopts::cxxopts)

unison_apply_language_subset(unison_replay_core)

unison_apply_warnings(unison_replay_core)

add_executable(unison_replay src/main.cpp)

target_link_libraries(unison_replay PRIVATE unison_replay_core arena_sim)

unison_apply_language_subset(unison_replay)

unison_apply_warnings(unison_replay)

add_test(NAME replay_lists_its_commands COMMAND unison_replay --help)

set_tests_properties(replay_lists_its_commands PROPERTIES PASS_REGULAR_EXPRESSION "verify" LABELS fast)

add_test(
    NAME replay_refuses_a_file_it_cannot_read
    COMMAND unison_replay verify "${CMAKE_CURRENT_BINARY_DIR}/no_such.replay"
)

set_tests_properties(replay_refuses_a_file_it_cannot_read PROPERTIES WILL_FAIL TRUE LABELS fast)

add_test(
    NAME replay_verifies_a_runner_replay
    COMMAND unison_replay verify "${CMAKE_BINARY_DIR}/tools/runner/runner_records_a_replay.replay"
)

set_tests_properties(replay_verifies_a_runner_replay PROPERTIES FIXTURES_REQUIRED runnerReplay LABELS fast)

add_test(
    NAME replay_plays_a_runner_replay
    COMMAND unison_replay play "${CMAKE_BINARY_DIR}/tools/runner/runner_records_a_replay.replay"
)

set_tests_properties(replay_plays_a_runner_replay PROPERTIES FIXTURES_REQUIRED runnerReplay LABELS fast)
