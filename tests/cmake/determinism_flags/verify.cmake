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

string(JSON probeCommand GET "${compileCommands}" 0 command)
string(REPLACE " " ";" commandTokens "${probeCommand}")

set(requiredFlags /fp:precise /arch:SSE2 /EHs-c- /GR-)
set(forbiddenFlags /fp:fast /fp:contract /EHsc /EHa /GR /arch:AVX /arch:AVX2)

foreach(flag IN LISTS requiredFlags)
    if(NOT "${flag}" IN_LIST commandTokens)
        message(FATAL_ERROR "missing ${flag} in: ${probeCommand}")
    endif()
endforeach()

if(NOT probeCommand MATCHES "/FI[^\"]*determinism_guard\.hpp")
    message(FATAL_ERROR "the determinism guard is not force-included in: ${probeCommand}")
endif()

foreach(flag IN LISTS forbiddenFlags)
    if("${flag}" IN_LIST commandTokens)
        message(FATAL_ERROR "forbidden ${flag} in: ${probeCommand}")
    endif()
endforeach()

message(STATUS "determinism flags verified: ${probeCommand}")
