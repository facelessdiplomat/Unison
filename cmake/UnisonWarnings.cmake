include_guard(GLOBAL)

# Holds one Unison target to the project's warning discipline, as errors: MSVC's conformance mode and
# level 4 warnings, or clang's common, extra, pedantic, shadowing and conversion warnings. Separate from the
# determinism contract so libraries outside it are held to the same standard, and so third-party
# targets can take the determinism flags without our warnings.
function(unison_apply_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /permissive- /W4 /WX)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(
            ${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Werror
        )
    else()
        message(FATAL_ERROR "unison_apply_warnings supports MSVC and clang only")
    endif()
endfunction()
