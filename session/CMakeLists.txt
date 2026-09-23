add_library(unison_session STATIC src/input_buffer.cpp)

target_include_directories(unison_session PUBLIC include)

target_link_libraries(unison_session PUBLIC unison_sim unison_net)

unison_apply_language_subset(unison_session)

unison_apply_warnings(unison_session)
