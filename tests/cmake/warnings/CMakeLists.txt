cmake_minimum_required(VERSION 3.25)

project(unison_warnings_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../../cmake")

include(UnisonWarnings)
include("${CMAKE_CURRENT_LIST_DIR}/../probe_expectations.cmake")

add_library(unison_warnings_probe STATIC probe.cpp)

unison_apply_warnings(unison_warnings_probe)

if(MSVC)
    set(expectedOptions /permissive- /W4 /WX)
    set(shadowDiagnostic C4456)
else()
    set(expectedOptions -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Werror)
    set(shadowDiagnostic "declaration shadows a local variable")
endif()

unison_expect_options(unison_warnings_probe ${expectedOptions})

get_target_property(warningOptions unison_warnings_probe COMPILE_OPTIONS)

unison_expect_rejected(shadow_probe.cpp "${shadowDiagnostic}" ${warningOptions})

if(NOT MSVC)
    unison_expect_rejected(narrowing_probe.cpp "implicit conversion loses integer precision" ${warningOptions})
    unison_expect_rejected(sign_change_probe.cpp "implicit conversion changes signedness" ${warningOptions})
endif()
