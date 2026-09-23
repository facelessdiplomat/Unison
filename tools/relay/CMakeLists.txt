add_library(unison_relay_core STATIC src/relay_options.cpp src/relay_rooms.cpp)

target_include_directories(unison_relay_core PUBLIC include)

target_link_libraries(unison_relay_core PUBLIC unison_net PRIVATE cxxopts::cxxopts)

unison_apply_language_subset(unison_relay_core)

unison_apply_warnings(unison_relay_core)

add_executable(unison_relay src/main.cpp)

target_link_libraries(unison_relay PRIVATE unison_relay_core)

unison_apply_language_subset(unison_relay)

unison_apply_warnings(unison_relay)

add_test(NAME relay_lists_its_options COMMAND unison_relay --help)

set_tests_properties(relay_lists_its_options PROPERTIES PASS_REGULAR_EXPRESSION "--peer-timeout" LABELS fast)

add_test(NAME relay_refuses_a_relay_without_peers COMMAND unison_relay --max-peers 0)

set_tests_properties(relay_refuses_a_relay_without_peers PROPERTIES WILL_FAIL TRUE LABELS fast)

add_test(NAME relay_listens_then_stops COMMAND unison_relay --bind 127.0.0.1 --port 0 --run-for 1)

set_tests_properties(
    relay_listens_then_stops PROPERTIES PASS_REGULAR_EXPRESSION "listening on 127\\.0\\.0\\.1:[1-9][0-9]*" LABELS fast
)
