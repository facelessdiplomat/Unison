cmake_minimum_required(VERSION 3.25)

project(unison_language_subset_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../../cmake")

include(UnisonLanguageSubset)

add_library(unison_language_subset_probe STATIC probe.cpp)

unison_apply_language_subset(unison_language_subset_probe)

get_target_property(subsetOptions unison_language_subset_probe COMPILE_OPTIONS)
get_target_property(subsetDefinitions unison_language_subset_probe COMPILE_DEFINITIONS)

if(MSVC)
    set(expectedOptions /EHs-c- /GR-)
    set(expectedDefinitions _HAS_EXCEPTIONS=0)
else()
    set(expectedOptions -fno-exceptions -fno-rtti)
    set(expectedDefinitions "")

    if(subsetDefinitions)
        message(FATAL_ERROR "the language subset defines ${subsetDefinitions}, which only the MSVC STL reads")
    endif()
endif()

foreach(option IN LISTS expectedOptions)
    if(NOT option IN_LIST subsetOptions)
        message(FATAL_ERROR "the language subset leaves out ${option}: ${subsetOptions}")
    endif()
endforeach()

foreach(definition IN LISTS expectedDefinitions)
    if(NOT definition IN_LIST subsetDefinitions)
        message(FATAL_ERROR "the language subset leaves out ${definition}: ${subsetDefinitions}")
    endif()
endforeach()

message(STATUS "the language subset carries ${subsetOptions}")

function(unison_expect_rejected source diagnostic)
    try_compile(
        compiledWithoutSubset
        SOURCES "${CMAKE_CURRENT_LIST_DIR}/${source}"
        OUTPUT_VARIABLE outputWithoutSubset
        NO_CACHE
    )

    if(NOT compiledWithoutSubset)
        message(FATAL_ERROR "${source} does not compile even with exceptions and RTTI on:\n${outputWithoutSubset}")
    endif()

    try_compile(
        compiledWithSubset
        SOURCES "${CMAKE_CURRENT_LIST_DIR}/${source}"
        COMPILE_DEFINITIONS ${ARGN}
        OUTPUT_VARIABLE outputWithSubset
        NO_CACHE
    )

    if(compiledWithSubset)
        message(FATAL_ERROR "${source} compiles under the language subset")
    endif()

    string(FIND "${outputWithSubset}" "${diagnostic}" diagnosticOffset)

    if(diagnosticOffset EQUAL -1)
        message(FATAL_ERROR "${source} failed under the language subset for another reason:\n${outputWithSubset}")
    endif()

    message(STATUS "the language subset rejects ${source}")
endfunction()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    unison_expect_rejected(throw_probe.cpp "cannot use 'throw' with exceptions disabled" ${subsetOptions})
    unison_expect_rejected(typeid_probe.cpp "use of typeid requires -frtti" ${subsetOptions})
endif()
