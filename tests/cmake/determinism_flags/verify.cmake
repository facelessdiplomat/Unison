cmake_minimum_required(VERSION 3.25)

include("${CMAKE_CURRENT_LIST_DIR}/../determinism_contract.cmake")

if(NOT DEFINED UNISON_BUILD_DIR)
    message(FATAL_ERROR "UNISON_BUILD_DIR is not set")
endif()

set(compileCommandsPath "${UNISON_BUILD_DIR}/compile_commands.json")
set(compilerIdPath "${UNISON_BUILD_DIR}/compiler_id.txt")

foreach(requiredPath IN ITEMS "${compileCommandsPath}" "${compilerIdPath}")
    if(NOT EXISTS "${requiredPath}")
        message(FATAL_ERROR "no ${requiredPath}")
    endif()
endforeach()

file(READ "${compilerIdPath}" compilerId)

file(READ "${compileCommandsPath}" compileCommands)
string(JSON entryCount LENGTH "${compileCommands}")

if(entryCount EQUAL 0)
    message(FATAL_ERROR "compile_commands.json has no entries")
endif()

string(JSON probeCommand GET "${compileCommands}" 0 command)
string(REPLACE " " ";" commandTokens "${probeCommand}")

if(compilerId STREQUAL "MSVC")
    set(requiredFlags /fp:precise /arch:SSE2 /EHs-c- /GR- -D_HAS_EXCEPTIONS=0)
    set(forbiddenFlags /fp:fast /fp:contract /EHsc /EHa /GR /arch:AVX /arch:AVX2)
    set(guardInclusion "/FI[^\"]*determinism_guard\.hpp")
else()
    set(requiredFlags -fno-fast-math -ffp-contract=off -fexcess-precision=standard -fno-exceptions -fno-rtti)
    set(forbiddenFlags ${unisonClangForbiddenFlags})
    set(guardInclusion "-include[^ ]*determinism_guard\\.hpp")
endif()

foreach(flag IN LISTS requiredFlags)
    if(NOT "${flag}" IN_LIST commandTokens)
        message(FATAL_ERROR "missing ${flag} in: ${probeCommand}")
    endif()
endforeach()

if(NOT probeCommand MATCHES "${guardInclusion}")
    message(FATAL_ERROR "the determinism guard is not force-included in: ${probeCommand}")
endif()

foreach(flag IN LISTS forbiddenFlags)
    if("${flag}" IN_LIST commandTokens)
        message(FATAL_ERROR "forbidden ${flag} in: ${probeCommand}")
    endif()
endforeach()

message(STATUS "determinism flags verified: ${probeCommand}")
