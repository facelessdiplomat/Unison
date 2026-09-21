include_guard(GLOBAL)

if(NOT DEFINED ENV{CPM_SOURCE_CACHE})
    set(CPM_SOURCE_CACHE
        "${CMAKE_SOURCE_DIR}/.cpm-cache"
        CACHE PATH "Directory where CPM stores dependency sources"
    )
endif()

include("${CMAKE_CURRENT_LIST_DIR}/CPM.cmake")

CPMAddPackage(
    NAME Catch2
    GITHUB_REPOSITORY catchorg/Catch2
    VERSION 3.16.0
    OPTIONS "CATCH_INSTALL_DOCS OFF" "CATCH_INSTALL_EXTRAS OFF" "CATCH_BUILD_TESTING OFF"
)

list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/extras")

CPMAddPackage(NAME EnTT GITHUB_REPOSITORY skypjack/entt VERSION 3.16.0)
