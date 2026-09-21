add_library(unison_view STATIC src/view.cpp)

target_include_directories(unison_view PUBLIC include)

target_link_libraries(unison_view PUBLIC unison_session)
