add_library(
    unison_sim STATIC
    src/advance_frame.cpp
    src/asset_hash.cpp
    src/asset_registry.cpp
    src/component_registry.cpp
    src/entity_lifecycle.cpp
    src/event_buffer.cpp
    src/frame_checksum.cpp
    src/frame_inputs.cpp
    src/frame_snapshot.cpp
    src/jolt_runtime.cpp
    src/physics_world.cpp
    src/pipeline_hash.cpp
    src/registry_clone.cpp
    src/signals.cpp
    src/system_pipeline.cpp
    src/sim.cpp
)

target_include_directories(unison_sim PUBLIC include)

target_link_libraries(unison_sim PUBLIC unison_core EnTT::EnTT Jolt boost_pfr)

unison_apply_determinism(unison_sim)

unison_apply_warnings(unison_sim)
