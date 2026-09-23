add_library(arena_sim OBJECT src/apply_input.cpp src/arena_simulation.cpp src/assets.cpp src/character_move.cpp src/components.cpp src/hits.cpp src/lifetimes.cpp src/match_rules.cpp src/respawn.cpp src/shot_path.cpp src/weapons.cpp)

target_include_directories(arena_sim PUBLIC include)

target_link_libraries(arena_sim PUBLIC unison_sim)

unison_apply_determinism(arena_sim)

unison_apply_warnings(arena_sim)

add_library(arena_view_console OBJECT view/src/text_map.cpp)

target_include_directories(arena_view_console PUBLIC view/include)

target_link_libraries(arena_view_console PUBLIC arena_sim)

unison_apply_language_subset(arena_view_console)

unison_apply_warnings(arena_view_console)
