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
    set(shownFlags "${floatingPointFlags}")

    if(shownFlags STREQUAL "")
        set(shownFlags "no flag at all")
    endif()

    try_compile(
        probeCompiled
        SOURCES "${guardProbeSource}"
        CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${guardIncludeDir}"
        OUTPUT_VARIABLE probeOutput
    )

    if(expectation STREQUAL "accepted" AND NOT probeCompiled)
        message(FATAL_ERROR "the guard rejected ${shownFlags}:\n${probeOutput}")
    endif()

    if(expectation STREQUAL "rejected")
        if(probeCompiled)
            message(FATAL_ERROR "the guard accepted ${shownFlags}:
${probeOutput}")
        endif()

        string(FIND "${probeOutput}" "unison: deterministic code requires" guardMessageOffset)
        if(guardMessageOffset EQUAL -1)
            message(FATAL_ERROR "${shownFlags} failed for another reason:\n${probeOutput}")
        endif()
    endif()

    message(STATUS "guard ${expectation} ${shownFlags}")
endfunction()

function(unison_expect_contraction_turned_off)
    cmake_path(SET guardHeader NORMALIZE "${guardIncludeDir}/unison/core/determinism_guard.hpp")
    set(contractionProbeSource "${CMAKE_CURRENT_LIST_DIR}/contraction_probe.cpp")

    try_run(
        exitWithoutGuard compiledWithoutGuard
        SOURCES "${contractionProbeSource}"
        COMPILE_DEFINITIONS -ffp-contract=on
        COMPILE_OUTPUT_VARIABLE outputWithoutGuard
        NO_CACHE
    )

    if(NOT compiledWithoutGuard)
        message(FATAL_ERROR "the contraction probe does not compile:\n${outputWithoutGuard}")
    endif()

    if(exitWithoutGuard EQUAL 0)
        message(STATUS "this machine never fuses a multiply and an add, so there is no contraction to turn off")
        return()
    endif()

    try_run(
        exitWithGuard compiledWithGuard
        SOURCES "${contractionProbeSource}"
        COMPILE_DEFINITIONS -ffp-contract=on "-include${guardHeader}"
        COMPILE_OUTPUT_VARIABLE outputWithGuard
        NO_CACHE
    )

    if(NOT compiledWithGuard)
        message(FATAL_ERROR "the contraction probe does not compile with the guard:\n${outputWithGuard}")
    endif()

    if(NOT exitWithGuard EQUAL 0)
        message(FATAL_ERROR "the guard leaves a multiply and an add fused under -ffp-contract=on")
    endif()

    message(STATUS "the guard keeps a multiply and an add apart under -ffp-contract=on")
endfunction()

if(MSVC)
    unison_expect_guard("/fp:precise" accepted)
    unison_expect_guard("" rejected)
    unison_expect_guard("/fp:fast" rejected)
    unison_expect_guard("/fp:precise /fp:contract" rejected)
    unison_expect_guard("/fp:strict" rejected)
else()
    unison_expect_guard("-fno-fast-math -ffp-contract=off -fexcess-precision=standard" accepted)
    unison_expect_guard("" accepted)
    unison_expect_guard("-ffast-math" rejected)
    unison_expect_guard("-ffinite-math-only" rejected)
    unison_expect_guard("-ffp-model=aggressive" rejected)
    unison_expect_guard("-Ofast" rejected)
    unison_expect_contraction_turned_off()
endif()
