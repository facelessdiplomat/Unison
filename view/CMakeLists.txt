add_library(unison_view STATIC src/event_dispatcher.cpp src/session_runner.cpp)

target_include_directories(unison_view PUBLIC include)

target_link_libraries(unison_view PUBLIC unison_session)

unison_apply_language_subset(unison_view)

unison_apply_warnings(unison_view)
