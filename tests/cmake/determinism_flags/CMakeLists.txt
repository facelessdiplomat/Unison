cmake_minimum_required(VERSION 3.25)

project(unison_determinism_probe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

string(REGEX REPLACE "/EH[a-z-]+" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../../cmake")

include(UnisonDeterminism)

add_library(unison_determinism_probe STATIC probe.cpp)

unison_apply_determinism(unison_determinism_probe)
