cmake_minimum_required(VERSION 3.25)

project(unison_determinism_guard_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(guardIncludeDir "${CMAKE_CURRENT_LIST_DIR}/../../../core/include")
set(guardProbeSource "${CMAKE_CURRENT_LIST_DIR}/guard_probe.cpp")

function(unison_expect_guard floatingPointFlags expectation)
    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
    set(CMAKE_CXX_FLAGS "${floatingPointFlags}")

    try_compile(
        probeCompiled
        SOURCES "${guardProbeSource}"
        CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${guardIncludeDir}"
        OUTPUT_VARIABLE probeOutput
    )

    if(expectation STREQUAL "accepted" AND NOT probeCompiled)
        message(FATAL_ERROR "the guard rejected ${floatingPointFlags}:\n${probeOutput}")
    endif()

    if(expectation STREQUAL "rejected")
        if(probeCompiled)
            message(FATAL_ERROR "the guard accepted ${floatingPointFlags}:
${probeOutput}")
        endif()

        string(FIND "${probeOutput}" "unison: deterministic code requires" guardMessageOffset)
        if(guardMessageOffset EQUAL -1)
            message(FATAL_ERROR "${floatingPointFlags} failed for another reason:\n${probeOutput}")
        endif()
    endif()

    message(STATUS "guard ${expectation} ${floatingPointFlags}")
endfunction()

unison_expect_guard("/fp:precise" accepted)
unison_expect_guard("" rejected)
unison_expect_guard("/fp:fast" rejected)
unison_expect_guard("/fp:precise /fp:contract" rejected)
unison_expect_guard("/fp:strict" rejected)
