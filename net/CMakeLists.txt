add_library(
    unison_net STATIC
    src/clock.cpp
    src/input_collector.cpp
    src/loopback_hub.cpp
    src/message_reader.cpp
    src/message_writer.cpp
    src/network_simulator.cpp
    src/relay_core.cpp
    src/session_config.cpp
)

target_include_directories(unison_net PUBLIC include)

target_link_libraries(unison_net PUBLIC unison_core enet)

unison_apply_language_subset(unison_net)

unison_apply_warnings(unison_net)
