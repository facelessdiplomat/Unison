include_guard(GLOBAL)

# Writes the JPH_ macros Jolt is built with into unison_jolt_config.hpp, so that a translation unit that
# includes Jolt's headers without Jolt's CMake usage requirements, as the Unreal plugin's do, sees the
# settings the library was built with. Jolt checks the ones that change its ABI itself, through
# JPH::VerifyJoltVersionID. The header is per configuration, since Jolt's macros are.
set(UNISON_JOLT_CONFIG_DIR "${CMAKE_BINARY_DIR}/generated/jolt_config/$<CONFIG>")

set(unisonJoltDefinitions "${UNISON_JOLT_CONFIG_DIR}/jolt_definitions.txt")
set(unisonJoltConfigHeader "${UNISON_JOLT_CONFIG_DIR}/unison_jolt_config.hpp")

file(GENERATE OUTPUT "${unisonJoltDefinitions}"
     CONTENT "$<JOIN:$<TARGET_PROPERTY:Jolt,INTERFACE_COMPILE_DEFINITIONS>,\n>\n"
)

add_custom_command(
    OUTPUT "${unisonJoltConfigHeader}"
    COMMAND ${CMAKE_COMMAND} -D "UNISON_JOLT_DEFINITIONS=${unisonJoltDefinitions}" -D
            "UNISON_JOLT_CONFIG_HEADER=${unisonJoltConfigHeader}" -P
            "${CMAKE_CURRENT_LIST_DIR}/WriteJoltConfig.cmake"
    DEPENDS "${unisonJoltDefinitions}" "${CMAKE_CURRENT_LIST_DIR}/WriteJoltConfig.cmake"
    VERBATIM
)

add_custom_target(unison_jolt_config ALL DEPENDS "${unisonJoltConfigHeader}")
