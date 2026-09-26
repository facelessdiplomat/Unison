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

add_test(
    NAME runner_records_a_replay
    COMMAND unison_runner --frames 120 --record "${CMAKE_CURRENT_BINARY_DIR}/runner_records_a_replay.replay"
)

set_tests_properties(
    runner_records_a_replay PROPERTIES FIXTURES_SETUP runnerReplay PASS_REGULAR_EXPRESSION "recorded the run into"
                                       LABELS fast
)

add_test(
    NAME runner_reports_a_replay_it_cannot_write
    COMMAND
        unison_runner --frames 120 --record "${CMAKE_CURRENT_BINARY_DIR}/no_such_folder/match.replay"
)

set_tests_properties(runner_reports_a_replay_it_cannot_write PROPERTIES WILL_FAIL TRUE LABELS fast)

add_test(
    NAME runner_dumps_the_snapshots_of_a_desync
    COMMAND unison_runner --frames 60 --fault 1 --dump-dir "${CMAKE_CURRENT_BINARY_DIR}/desync_dumps"
)

set_tests_properties(
    runner_dumps_the_snapshots_of_a_desync PROPERTIES FIXTURES_SETUP desyncDumps PASS_REGULAR_EXPRESSION
                                                      "dumped the snapshot of the desync" LABELS fast
)

set(UNISON_PROFILE_SECONDS 5)

foreach(players IN ITEMS 2 4 8)
    foreach(tickRate IN ITEMS 30 60)
        math(EXPR frames "${tickRate} * ${UNISON_PROFILE_SECONDS}")

        foreach(loss IN ITEMS 0 5 20)
            set(profile "runner_${players}_players_at_${tickRate}_hz_losing_${loss}_percent")

            add_test(
                NAME ${profile}
                COMMAND
                    unison_runner --players ${players} --frames ${frames} --tick-rate ${tickRate} --latency 120 --jitter
                    30 --loss ${loss}
            )

            set_tests_properties(${profile} PROPERTIES LABELS profile)
        endforeach()
    endforeach()
endforeach()
