add_library(unison_net STATIC src/net.cpp)

target_include_directories(unison_net PUBLIC include)

target_link_libraries(unison_net PUBLIC unison_core enet)

unison_apply_warnings(unison_net)
