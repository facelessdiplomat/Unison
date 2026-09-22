include_guard(GLOBAL)

include(UnisonLanguageSubset)

# The instruction set every deterministic library and Jolt are built for. SSE2 is the baseline the
# charter requires of a v1 build; AVX2 exists so that the comparison of DESIGN.md Q2 can be run
# again. Both halves have to agree, because Jolt changes its own types with them.
set(UNISON_INSTRUCTION_SET
    "SSE2"
    CACHE STRING "Instruction set for the deterministic libraries and Jolt: SSE2 or AVX2"
)

set_property(CACHE UNISON_INSTRUCTION_SET PROPERTY STRINGS SSE2 AVX2)

if(NOT UNISON_INSTRUCTION_SET MATCHES "^(SSE2|AVX2)$")
    message(FATAL_ERROR "UNISON_INSTRUCTION_SET must be SSE2 or AVX2, not ${UNISON_INSTRUCTION_SET}")
endif()

# Applies the compiler contract of DESIGN.md 7.1 to one MSVC x64 target and force-includes the
# determinism guard into it. The exception and RTTI half of that contract is shared with the targets
# that carry no determinism flags, so it lives in unison_apply_language_subset. The default /EHsc is
# removed project-wide in the root CMakeLists, so a target carries a single exception setting instead
# of overriding one on the command line.
function(unison_apply_determinism target)
    if(NOT MSVC)
        message(FATAL_ERROR "unison_apply_determinism supports MSVC only")
    endif()

    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "unison_apply_determinism requires an x64 target")
    endif()

    cmake_path(SET determinismGuardHeader NORMALIZE
               "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../core/include/unison/core/determinism_guard.hpp")

    if(NOT EXISTS "${determinismGuardHeader}")
        message(FATAL_ERROR "the determinism guard header is missing at ${determinismGuardHeader}")
    endif()

    unison_apply_language_subset(${target})

    if(UNISON_INSTRUCTION_SET STREQUAL "AVX2")
        set(instructionSetFlag /arch:AVX2)
    else()
        set(instructionSetFlag /arch:SSE2)
    endif()

    target_compile_options(
        ${target}
        PRIVATE /fp:precise
                ${instructionSetFlag}
                "/FI${determinismGuardHeader}"
    )
endfunction()
