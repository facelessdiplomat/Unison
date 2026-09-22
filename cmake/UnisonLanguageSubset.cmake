include_guard(GLOBAL)

# The language subset of DESIGN.md 7.1 that every Unison target compiles with: no exceptions and no
# RTTI, matching Unreal's own defaults so the libraries can be linked into the plugin unchanged.
# /EHs-c- on its own leaves the MSVC STL emitting try/catch that cannot unwind, so _HAS_EXCEPTIONS=0,
# the STL's own switch for a build without exceptions, belongs with it; Jolt sets the same macro for
# itself, and a target without it disagrees with Jolt about the types they share.
function(unison_apply_language_subset target)
    if(NOT MSVC)
        message(FATAL_ERROR "unison_apply_language_subset supports MSVC only")
    endif()

    target_compile_options(${target} PRIVATE /EHs-c- /GR-)

    target_compile_definitions(${target} PRIVATE _HAS_EXCEPTIONS=0)
endfunction()
