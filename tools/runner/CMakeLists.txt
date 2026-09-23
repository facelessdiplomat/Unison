add_library(
    unison_runner_core STATIC
    src/checksum_ledger.cpp
    src/checksum_wiretap.cpp
    src/client_outcome.cpp
    src/rollback_summary.cpp
    src/run_outcome.cpp
    src/runner_options.cpp
)

target_include_directories(unison_runner_core PUBLIC include)

target_link_libraries(unison_runner_core PUBLIC unison_net unison_session PRIVATE cxxopts::cxxopts)

unison_apply_language_subset(unison_runner_core)

unison_apply_warnings(unison_runner_core)

add_library(unison_runner_match OBJECT src/runner_client.cpp src/runner_match.cpp src/scripted_player.cpp)

target_link_libraries(unison_runner_match PUBLIC unison_runner_core unison_view arena_sim)

unison_apply_language_subset(unison_runner_match)

unison_apply_warnings(unison_runner_match)

add_executable(unison_runner src/main.cpp)

target_link_libraries(unison_runner PRIVATE unison_runner_match arena_sim)

unison_apply_language_subset(unison_runner)

unison_apply_warnings(unison_runner)

add_test(NAME runner_lists_its_options COMMAND unison_runner --help)

set_tests_properties(runner_lists_its_options PROPERTIES PASS_REGULAR_EXPRESSION "--checksum-interval" LABELS fast)

add_test(NAME runner_refuses_a_match_without_players COMMAND unison_runner --players 0)

set_tests_properties(runner_refuses_a_match_without_players PROPERTIES WILL_FAIL TRUE LABELS fast)

add_test(NAME runner_plays_two_players_for_600_frames COMMAND unison_runner --players 2 --frames 600)

set_tests_properties(
    runner_plays_two_players_for_600_frames PROPERTIES PASS_REGULAR_EXPRESSION "2 players verified 600 frames" LABELS fast
)
