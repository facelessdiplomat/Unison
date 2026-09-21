cmake_minimum_required(VERSION 3.25)

project(unison VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(Dependencies)

include(UnisonDeterminism)

add_subdirectory(core)
add_subdirectory(sim)
add_subdirectory(net)
add_subdirectory(session)
add_subdirectory(view)
