include_guard(GLOBAL)

include(UnisonLanguageSubset)

# The architecture the deterministic libraries and Jolt are built for and the instruction sets allowed
# on it. SSE2 is the x86-64 baseline the charter requires of a v1 build, and AVX2 exists so that the
# comparison of DESIGN.md Q2 can be run again; arm64 has NEON alone. The libraries and Jolt have to
# agree on it, because Jolt changes its own types with it.
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|ARM64|aarch64)$")
    set(unisonArchitecture arm64)
    set(unisonInstructionSets NEON)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(AMD64|x86_64)$")
    set(unisonArchitecture x86-64)
    set(unisonInstructionSets SSE2 AVX2)
else()
    message(FATAL_ERROR "the deterministic libraries build for x86-64 and arm64, not ${CMAKE_SYSTEM_PROCESSOR}")
endif()

list(GET unisonInstructionSets 0 unisonDefaultInstructionSet)
list(JOIN unisonInstructionSets " or " unisonAllowedInstructionSets)

set(UNISON_INSTRUCTION_SET
    "${unisonDefaultInstructionSet}"
    CACHE STRING "Instruction set for the deterministic libraries and Jolt: ${unisonAllowedInstructionSets}"
)

set_property(CACHE UNISON_INSTRUCTION_SET PROPERTY STRINGS ${unisonInstructionSets})

if(NOT UNISON_INSTRUCTION_SET IN_LIST unisonInstructionSets)
    message(FATAL_ERROR "UNISON_INSTRUCTION_SET must be ${unisonAllowedInstructionSets} on ${unisonArchitecture}, "
                        "not ${UNISON_INSTRUCTION_SET}")
endif()

# Applies the compiler contract of DESIGN.md 7.1 to one 64-bit target and force-includes the determinism
# guard into it: /fp:precise and the /arch: baseline on MSVC; on clang -fno-fast-math and
# -ffp-contract=off, since clang fuses a * b + c by default, and nothing beyond the baseline but AVX2.
# The exception and RTTI half of that contract is shared with the targets that carry no determinism
# flags, so it lives in unison_apply_language_subset. The default /EHsc is removed project-wide in the
# root CMakeLists, so a target carries a single exception setting instead of overriding one on the
# command line.
function(unison_apply_determinism target)
    if(NOT MSVC AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "unison_apply_determinism supports MSVC and clang only")
    endif()

    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "unison_apply_determinism requires a 64-bit target")
    endif()

    cmake_path(SET determinismGuardHeader NORMALIZE
               "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../core/include/unison/core/determinism_guard.hpp")

    if(NOT EXISTS "${determinismGuardHeader}")
        message(FATAL_ERROR "the determinism guard header is missing at ${determinismGuardHeader}")
    endif()

    unison_apply_language_subset(${target})

    if(MSVC)
        target_compile_options(
            ${target}
            PRIVATE /fp:precise
                    /arch:${UNISON_INSTRUCTION_SET}
                    "/FI${determinismGuardHeader}"
        )
    else()
        if(UNISON_INSTRUCTION_SET STREQUAL "AVX2")
            set(instructionSetFlags -mavx2)
        else()
            set(instructionSetFlags "")
        endif()

        target_compile_options(
            ${target}
            PRIVATE -fno-fast-math
                    -ffp-contract=off
                    -fexcess-precision=standard
                    ${instructionSetFlags}
                    "-include${determinismGuardHeader}"
        )
    endif()
endfunction()
