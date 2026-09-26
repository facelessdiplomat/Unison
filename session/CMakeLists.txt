add_library(
    unison_session STATIC
    src/event_history.cpp
    src/input_buffer.cpp
    src/input_timeline.cpp
    src/local_input.cpp
    src/networked_session.cpp
    src/repeat_last_input_predictor.cpp
    src/replay_player.cpp
    src/replay_reader.cpp
    src/replay_writer.cpp
    src/rollback_stats.cpp
    src/session.cpp
    src/snapshot_ring.cpp
    src/time_sync.cpp
)

target_include_directories(unison_session PUBLIC include)

target_link_libraries(unison_session PUBLIC unison_sim unison_net)

unison_apply_language_subset(unison_session)

unison_apply_warnings(unison_session)
