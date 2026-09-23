add_library(unison_runner_options STATIC src/runner_options.cpp)

target_include_directories(unison_runner_options PUBLIC include)

target_link_libraries(unison_runner_options PUBLIC unison_net PRIVATE cxxopts::cxxopts)

unison_apply_language_subset(unison_runner_options)

unison_apply_warnings(unison_runner_options)

add_executable(unison_runner src/main.cpp)

target_link_libraries(unison_runner PRIVATE unison_runner_options)

unison_apply_language_subset(unison_runner)

unison_apply_warnings(unison_runner)

add_test(NAME runner_lists_its_options COMMAND unison_runner --help)

set_tests_properties(runner_lists_its_options PROPERTIES PASS_REGULAR_EXPRESSION "--checksum-interval" LABELS fast)

add_test(NAME runner_refuses_a_match_without_players COMMAND unison_runner --players 0)

set_tests_properties(runner_refuses_a_match_without_players PROPERTIES WILL_FAIL TRUE LABELS fast)
