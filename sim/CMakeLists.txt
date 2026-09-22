add_library(unison_sim STATIC src/component_registry.cpp src/event_buffer.cpp src/frame_checksum.cpp src/frame_snapshot.cpp src/registry_clone.cpp src/sim.cpp)

target_include_directories(unison_sim PUBLIC include)

target_link_libraries(unison_sim PUBLIC unison_core EnTT::EnTT Jolt boost_pfr)

unison_apply_determinism(unison_sim)

unison_apply_warnings(unison_sim)
