include_guard(GLOBAL)

# Applies the floating-point, exception, RTTI and warning contract of DESIGN.md 7.1 to one MSVC x64
# target, and removes the default /EHsc from the calling directory so that the target keeps a single
# exception setting instead of overriding one on the command line.
function(unison_apply_determinism target)
    if(NOT MSVC)
        message(FATAL_ERROR "unison_apply_determinism supports MSVC only")
    endif()

    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "unison_apply_determinism requires an x64 target")
    endif()

    string(REGEX REPLACE "/EH[a-z-]+" "" flagsWithoutExceptions "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "${flagsWithoutExceptions}" PARENT_SCOPE)

    target_compile_options(
        ${target}
        PRIVATE /fp:precise
                /arch:SSE2
                /EHs-c-
                /GR-
                /permissive-
                /W4
                /WX
    )
endfunction()
