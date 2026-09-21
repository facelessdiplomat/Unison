set(determinismFlagsBinaryDir "${CMAKE_CURRENT_BINARY_DIR}/determinism_flags")

add_test(
    NAME determinism_flags_project
    COMMAND ${CMAKE_COMMAND} -S "${CMAKE_CURRENT_LIST_DIR}/cmake/determinism_flags" -B
            "${determinismFlagsBinaryDir}" -G "${CMAKE_GENERATOR}"
)

set_tests_properties(determinism_flags_project PROPERTIES FIXTURES_SETUP determinismFlagsProject)

add_test(
    NAME determinism_flags
    COMMAND ${CMAKE_COMMAND} -D "UNISON_BUILD_DIR=${determinismFlagsBinaryDir}" -P
            "${CMAKE_CURRENT_LIST_DIR}/cmake/determinism_flags/verify.cmake"
)

set_tests_properties(determinism_flags PROPERTIES FIXTURES_REQUIRED determinismFlagsProject)

add_test(
    NAME determinism_guard
    COMMAND ${CMAKE_COMMAND} -S "${CMAKE_CURRENT_LIST_DIR}/cmake/determinism_guard" -B
            "${CMAKE_CURRENT_BINARY_DIR}/determinism_guard" -G "${CMAKE_GENERATOR}"
)

add_test(
    NAME module_determinism
    COMMAND ${CMAKE_COMMAND} -D "UNISON_BUILD_DIR=${CMAKE_BINARY_DIR}" -P
            "${CMAKE_CURRENT_LIST_DIR}/cmake/module_determinism/verify.cmake"
)
