include_guard(GLOBAL)

function(unison_expect_options target)
    get_target_property(options ${target} COMPILE_OPTIONS)

    foreach(option IN LISTS ARGN)
        if(NOT option IN_LIST options)
            message(FATAL_ERROR "${target} is built without ${option}: ${options}")
        endif()
    endforeach()

    message(STATUS "${target} carries ${options}")
endfunction()

function(unison_expect_rejected source diagnostic)
    try_compile(
        compiledWithoutOptions
        SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${source}"
        OUTPUT_VARIABLE outputWithoutOptions
        NO_CACHE
    )

    if(NOT compiledWithoutOptions)
        message(FATAL_ERROR "${source} does not compile even without ${ARGN}:\n${outputWithoutOptions}")
    endif()

    try_compile(
        compiledWithOptions
        SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${source}"
        COMPILE_DEFINITIONS ${ARGN}
        OUTPUT_VARIABLE outputWithOptions
        NO_CACHE
    )

    if(compiledWithOptions)
        message(FATAL_ERROR "${source} compiles under ${ARGN}")
    endif()

    string(FIND "${outputWithOptions}" "${diagnostic}" diagnosticOffset)

    if(diagnosticOffset EQUAL -1)
        message(FATAL_ERROR "${source} failed under ${ARGN} for another reason than ${diagnostic}:\n${outputWithOptions}")
    endif()

    message(STATUS "${source} is refused under ${ARGN}")
endfunction()
