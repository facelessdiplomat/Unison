add_library(unison_sim STATIC src/sim.cpp)

target_include_directories(unison_sim PUBLIC include)

target_link_libraries(unison_sim PUBLIC unison_core EnTT::EnTT)

unison_apply_determinism(unison_sim)
