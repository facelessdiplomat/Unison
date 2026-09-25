include_guard(GLOBAL)

# The language subset of DESIGN.md 7.1 that every Unison target compiles with: no exceptions and no
# RTTI, matching Unreal's own defaults so the libraries can be linked into the plugin unchanged.
# /EHs-c- on its own leaves the MSVC STL emitting try/catch that cannot unwind, so _HAS_EXCEPTIONS=0,
# the STL's own switch for a build without exceptions, belongs with it; Jolt sets the same macro for
# itself, and a target without it disagrees with Jolt about the types they share. Clang needs only
# -fno-exceptions and -fno-rtti, which libc++ follows by itself.
function(unison_apply_language_subset target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /EHs-c- /GR-)

        target_compile_definitions(${target} PRIVATE _HAS_EXCEPTIONS=0)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${target} PRIVATE -fno-exceptions -fno-rtti)
    else()
        message(FATAL_ERROR "unison_apply_language_subset supports MSVC and clang only")
    endif()
endfunction()
