cmake_minimum_required(VERSION 3.25)

foreach(requiredVariable IN ITEMS UNISON_INSTRUCTION_SET UNISON_BUILD_DIR UNISON_CXX_COMPILER_ID)
    if(NOT DEFINED ${requiredVariable})
        message(FATAL_ERROR "${requiredVariable} is not set")
    endif()
endforeach()

include("${CMAKE_CURRENT_LIST_DIR}/../determinism_contract.cmake")

set(compileCommandsPath "${UNISON_BUILD_DIR}/compile_commands.json")

if(NOT EXISTS "${compileCommandsPath}")
    message(FATAL_ERROR "no compile_commands.json at ${compileCommandsPath}")
endif()

file(READ "${compileCommandsPath}" compileCommands)
string(JSON entryCount LENGTH "${compileCommands}")

if(entryCount EQUAL 0)
    message(FATAL_ERROR "compile_commands.json has no entries")
endif()

if(UNISON_CXX_COMPILER_ID STREQUAL "MSVC")
    set(determinismFlags "/fp:precise")
    set(instructionSetFlags "/arch:${UNISON_INSTRUCTION_SET}")
    set(warningFlags "/permissive-" "/W4" "/WX")
    set(languageSubsetFlags "/EHs-c-" "/GR-" "_HAS_EXCEPTIONS=0")
    set(exceptionFreeFlag "_HAS_EXCEPTIONS=0")
    set(exceptionsOnFlag "/EHsc")
    set(forbiddenFlags "")
else()
    set(determinismFlags "-fno-fast-math" "-ffp-contract=off" "-fexcess-precision=standard")
    set(instructionSetFlags "")
    set(warningFlags "-Wall" "-Wextra" "-Wpedantic" "-Wshadow" "-Werror")
    set(languageSubsetFlags "-fno-exceptions" "-fno-rtti")
    set(exceptionFreeFlag "-fno-exceptions")
    set(exceptionsOnFlag "-fexceptions")
    set(forbiddenFlags ${unisonClangForbiddenFlags})
endif()

function(unison_require label entryCommand)
    foreach(flag IN LISTS ARGN)
        if(NOT entryCommand MATCHES "${flag}")
            message(FATAL_ERROR "${label} is built without ${flag}: ${entryCommand}")
        endif()
    endforeach()
endfunction()

function(unison_forbid label entryCommand)
    foreach(flag IN LISTS ARGN)
        if(entryCommand MATCHES "${flag}")
            message(FATAL_ERROR "${label} is built with ${flag}: ${entryCommand}")
        endif()
    endforeach()
endfunction()

set(deterministicModules core sim arena)
set(plainModules session net view console relay runner)
set(seenModules "")
set(joltSeen FALSE)
set(headerCheckSeen FALSE)
set(canarySeen FALSE)
set(testExecutableSeen FALSE)

math(EXPR lastEntry "${entryCount} - 1")

foreach(entryIndex RANGE ${lastEntry})
    string(JSON entryFile GET "${compileCommands}" ${entryIndex} file)
    string(JSON entryCommand GET "${compileCommands}" ${entryIndex} command)

    if(entryFile MATCHES "joltphysics")
        set(joltSeen TRUE)

        unison_require(jolt "${entryCommand}" ${determinismFlags} ${instructionSetFlags} "JPH_CROSS_PLATFORM_DETERMINISTIC")
        unison_forbid(
            jolt "${entryCommand}" ${exceptionsOnFlag} ${forbiddenFlags} "JPH_USE_DX12" "JPH_USE_VK" "JPH_USE_MTL"
            "JPH_USE_CPU_COMPUTE"
        )
    endif()

    if(entryFile MATCHES "/tests/compile/")
        set(headerCheckSeen TRUE)

        unison_require(
            "the core header check" "${entryCommand}" ${determinismFlags} ${warningFlags} ${exceptionFreeFlag}
            "determinism_guard\\.hpp"
        )
        unison_forbid("the core header check" "${entryCommand}" ${forbiddenFlags})
    endif()

    if(entryFile MATCHES "/tests/support/multiply_then_add\\.cpp")
        set(canarySeen TRUE)

        unison_require("the determinism canary" "${entryCommand}" ${determinismFlags} "determinism_guard\\.hpp")
        unison_forbid("the determinism canary" "${entryCommand}" ${forbiddenFlags})
    endif()

    if(entryFile MATCHES "/tests/(core|sim|net|session|view|arena|tools)/")
        set(testExecutableSeen TRUE)

        if(UNISON_CXX_COMPILER_ID STREQUAL "MSVC")
            unison_require("the test executable" "${entryCommand}" "/EHsc")
        else()
            unison_require("the test executable" "${entryCommand}" "-ffp-contract=off")
            unison_forbid("the test executable" "${entryCommand}" "/EHsc")
        endif()

        unison_forbid("the test executable" "${entryCommand}" ${exceptionFreeFlag})
    endif()

    foreach(module IN LISTS deterministicModules plainModules)
        if(NOT entryFile MATCHES "/${module}/src/")
            continue()
        endif()

        list(APPEND seenModules ${module})

        unison_require(${module} "${entryCommand}" ${warningFlags})

        if(module IN_LIST deterministicModules)
            unison_require(${module} "${entryCommand}" ${determinismFlags} ${exceptionFreeFlag} "determinism_guard\\.hpp")
            unison_forbid(${module} "${entryCommand}" ${exceptionsOnFlag} ${forbiddenFlags})
        else()
            unison_forbid(${module} "${entryCommand}" ${determinismFlags} "determinism_guard\\.hpp")
            unison_require(${module} "${entryCommand}" ${languageSubsetFlags})
        endif()
    endforeach()
endforeach()

foreach(module IN LISTS deterministicModules plainModules)
    if(NOT module IN_LIST seenModules)
        message(FATAL_ERROR "no compile command for module ${module}")
    endif()
endforeach()

if(NOT joltSeen)
    message(FATAL_ERROR "no compile command for jolt")
endif()

if(NOT headerCheckSeen)
    message(FATAL_ERROR "no compile command for the core header check")
endif()

if(NOT canarySeen)
    message(FATAL_ERROR "no compile command for the determinism canary")
endif()

if(NOT testExecutableSeen)
    message(FATAL_ERROR "no compile command for the test executable")
endif()

message(STATUS "determinism applied to ${deterministicModules} and jolt for ${UNISON_INSTRUCTION_SET}, kept off ${plainModules}")
