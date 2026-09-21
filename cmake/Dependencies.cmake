include_guard(GLOBAL)

if(NOT DEFINED ENV{CPM_SOURCE_CACHE})
    set(CPM_SOURCE_CACHE
        "${CMAKE_SOURCE_DIR}/.cpm-cache"
        CACHE PATH "Directory where CPM stores dependency sources"
    )
endif()

include("${CMAKE_CURRENT_LIST_DIR}/CPM.cmake")
