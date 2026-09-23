add_library(unison_console_core STATIC src/console_options.cpp src/screen.cpp src/status_line.cpp)

target_include_directories(unison_console_core PUBLIC include)

target_link_libraries(unison_console_core PUBLIC unison_session PRIVATE cxxopts::cxxopts)

unison_apply_language_subset(unison_console_core)

unison_apply_warnings(unison_console_core)

add_library(unison_console_arena OBJECT src/arena_controls.cpp)

target_include_directories(unison_console_arena PUBLIC include)

target_link_libraries(unison_console_arena PUBLIC arena_sim)

unison_apply_language_subset(unison_console_arena)

unison_apply_warnings(unison_console_arena)

add_executable(unison_console src/console_keyboard.cpp src/console_screen.cpp src/main.cpp)

target_link_libraries(
    unison_console PRIVATE arena_view_console unison_console_arena unison_console_core unison_view arena_sim
)

target_compile_definitions(unison_console PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

unison_apply_language_subset(unison_console)

unison_apply_warnings(unison_console)

add_test(NAME console_lists_its_options COMMAND unison_console --help)

set_tests_properties(console_lists_its_options PROPERTIES PASS_REGULAR_EXPRESSION "--players" LABELS fast)

add_test(NAME console_refuses_a_match_without_players COMMAND unison_console --players 0)

set_tests_properties(console_refuses_a_match_without_players PROPERTIES WILL_FAIL TRUE LABELS fast)
