cmake_minimum_required(VERSION 3.25)

if(NOT DEFINED UNISON_BUILD_DIR)
    message(FATAL_ERROR "UNISON_BUILD_DIR is not set")
endif()

set(compileCommandsPath "${UNISON_BUILD_DIR}/compile_commands.json")

if(NOT EXISTS "${compileCommandsPath}")
    message(FATAL_ERROR "no compile_commands.json at ${compileCommandsPath}")
endif()

file(READ "${compileCommandsPath}" compileCommands)
string(JSON entryCount LENGTH "${compileCommands}")

if(entryCount EQUAL 0)
    message(FATAL_ERROR "compile_commands.json has no entries")
endif()

set(deterministicModules core sim)
set(plainModules session net view)
set(seenModules "")
set(joltSeen FALSE)

math(EXPR lastEntry "${entryCount} - 1")

foreach(entryIndex RANGE ${lastEntry})
    string(JSON entryFile GET "${compileCommands}" ${entryIndex} file)
    string(JSON entryCommand GET "${compileCommands}" ${entryIndex} command)

    if(entryFile MATCHES "joltphysics")
        set(joltSeen TRUE)

        foreach(flag IN ITEMS "/fp:precise" "/arch:SSE2")
            if(NOT entryCommand MATCHES "${flag}")
                message(FATAL_ERROR "jolt is built without ${flag}")
            endif()
        endforeach()

        if(entryCommand MATCHES "/EHsc")
            message(FATAL_ERROR "jolt is built with exceptions enabled")
        endif()
    endif()

    foreach(module IN LISTS deterministicModules plainModules)
        if(NOT entryFile MATCHES "/${module}/src/")
            continue()
        endif()

        list(APPEND seenModules ${module})

        if(module IN_LIST deterministicModules)
            if(NOT entryCommand MATCHES "/fp:precise")
                message(FATAL_ERROR "${module} is built without /fp:precise: ${entryCommand}")
            endif()

            if(entryCommand MATCHES "/EHsc")
                message(FATAL_ERROR "${module} still carries the default /EHsc")
            endif()

            if(NOT entryCommand MATCHES "determinism_guard\.hpp")
                message(FATAL_ERROR "${module} does not force-include the determinism guard")
            endif()
        elseif(entryCommand MATCHES "/fp:precise")
            message(FATAL_ERROR "${module} carries determinism flags it should not have")
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

message(STATUS "determinism applied to ${deterministicModules} and jolt, kept off ${plainModules}")
