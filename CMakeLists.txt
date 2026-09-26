cmake_minimum_required(VERSION 3.25)

project(unison VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE)
    set(CMAKE_LINK_LIBRARIES_STRATEGY REORDER_FREELY)
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

string(REGEX REPLACE "/EH[a-z-]+" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(UnisonDeterminism)
include(UnisonLanguageSubset)
include(UnisonWarnings)

include(Dependencies)

enable_testing()

add_subdirectory(core)
add_subdirectory(sim)
add_subdirectory(net)
add_subdirectory(session)
add_subdirectory(view)

add_subdirectory(samples/arena)

add_subdirectory(tools/console)
add_subdirectory(tools/relay)
add_subdirectory(tools/replay)
add_subdirectory(tools/runner)

add_subdirectory(tests)
