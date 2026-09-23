add_library(
    unison_net STATIC
    src/checksum_referee.cpp
    src/clock.cpp
    src/confirmed_log.cpp
    src/enet_transport.cpp
    src/input_collector.cpp
    src/loopback_hub.cpp
    src/match_clock.cpp
    src/message_reader.cpp
    src/message_writer.cpp
    src/network_simulator.cpp
    src/outbox.cpp
    src/relay_core.cpp
    src/roster.cpp
    src/session_config.cpp
)

target_include_directories(unison_net PUBLIC include)

target_link_libraries(unison_net PUBLIC unison_core enet)

unison_apply_language_subset(unison_net)

unison_apply_warnings(unison_net)
