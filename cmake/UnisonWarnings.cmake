include_guard(GLOBAL)

# Holds one Unison target to the project's warning discipline: MSVC conformance mode and level 4
# warnings as errors. Separate from the determinism contract so libraries outside it are held to the
# same standard, and so third-party targets can take the determinism flags without our warnings.
function(unison_apply_warnings target)
    if(NOT MSVC)
        message(FATAL_ERROR "unison_apply_warnings supports MSVC only")
    endif()

    target_compile_options(${target} PRIVATE /permissive- /W4 /WX)
endfunction()
