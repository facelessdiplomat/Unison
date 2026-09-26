# Turns the list of Jolt's compile definitions, one a line, into a header of #define lines, keeping the
# JPH_ macros alone: NDEBUG and the like are the host's to choose.
file(STRINGS "${UNISON_JOLT_DEFINITIONS}" definitions)

set(header "#pragma once\n")

foreach(definition IN LISTS definitions)
    if(definition MATCHES "^(JPH_[A-Za-z0-9_]+)=(.*)$")
        string(APPEND header "#define ${CMAKE_MATCH_1} ${CMAKE_MATCH_2}\n")
    elseif(definition MATCHES "^(JPH_[A-Za-z0-9_]+)$")
        string(APPEND header "#define ${CMAKE_MATCH_1}\n")
    endif()
endforeach()

file(WRITE "${UNISON_JOLT_CONFIG_HEADER}" "${header}")
