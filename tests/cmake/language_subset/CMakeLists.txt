cmake_minimum_required(VERSION 3.25)

project(unison_language_subset_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../../cmake")

include(UnisonLanguageSubset)
include("${CMAKE_CURRENT_LIST_DIR}/../probe_expectations.cmake")

add_library(unison_language_subset_probe STATIC probe.cpp)

unison_apply_language_subset(unison_language_subset_probe)

get_target_property(subsetOptions unison_language_subset_probe COMPILE_OPTIONS)
get_target_property(subsetDefinitions unison_language_subset_probe COMPILE_DEFINITIONS)

if(MSVC)
    unison_expect_options(unison_language_subset_probe /EHs-c- /GR-)

    if(NOT "_HAS_EXCEPTIONS=0" IN_LIST subsetDefinitions)
        message(FATAL_ERROR "the language subset leaves out _HAS_EXCEPTIONS=0: ${subsetDefinitions}")
    endif()
else()
    unison_expect_options(unison_language_subset_probe -fno-exceptions -fno-rtti)

    if(subsetDefinitions)
        message(FATAL_ERROR "the language subset defines ${subsetDefinitions}, which only the MSVC STL reads")
    endif()

    unison_expect_rejected(throw_probe.cpp "cannot use 'throw' with exceptions disabled" ${subsetOptions})
    unison_expect_rejected(typeid_probe.cpp "use of typeid requires -frtti" ${subsetOptions})
endif()
