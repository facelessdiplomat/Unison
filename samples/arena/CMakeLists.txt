add_library(arena_sim OBJECT src/components.cpp)

target_include_directories(arena_sim PUBLIC include)

target_link_libraries(arena_sim PUBLIC unison_sim)

unison_apply_determinism(arena_sim)

unison_apply_warnings(arena_sim)
