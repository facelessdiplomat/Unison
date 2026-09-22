include_guard(GLOBAL)

# Applies the compiler contract of DESIGN.md 7.1 to one MSVC x64 target and force-includes the
# determinism guard into it. The default /EHsc is removed project-wide in the root CMakeLists, so the
# target carries a single exception setting instead of overriding one on the command line.
# _HAS_EXCEPTIONS=0 is the MSVC STL's own switch for a build without exceptions: without it the STL
# still compiles try/catch that cannot unwind, and the target disagrees with Jolt, which sets the
# macro for itself. The two belong together, so they are applied together.
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

    target_compile_options(
        ${target}
        PRIVATE /fp:precise
                /arch:SSE2
                /EHs-c-
                /GR-
                "/FI${determinismGuardHeader}"
    )

    target_compile_definitions(${target} PRIVATE _HAS_EXCEPTIONS=0)
endfunction()
