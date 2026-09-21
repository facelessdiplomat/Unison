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

math(EXPR lastEntry "${entryCount} - 1")

foreach(entryIndex RANGE ${lastEntry})
    string(JSON entryFile GET "${compileCommands}" ${entryIndex} file)
    string(JSON entryCommand GET "${compileCommands}" ${entryIndex} command)

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

message(STATUS "determinism applied to ${deterministicModules}, kept off ${plainModules}")
