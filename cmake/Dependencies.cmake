include_guard(GLOBAL)

if(NOT DEFINED ENV{CPM_SOURCE_CACHE})
    set(CPM_SOURCE_CACHE
        "${CMAKE_SOURCE_DIR}/.cpm-cache"
        CACHE PATH "Directory where CPM stores dependency sources"
    )
endif()

include("${CMAKE_CURRENT_LIST_DIR}/UnisonDeterminism.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/CPM.cmake")

CPMAddPackage(
    NAME Catch2
    GITHUB_REPOSITORY catchorg/Catch2
    VERSION 3.16.0
    OPTIONS "CATCH_INSTALL_DOCS OFF" "CATCH_INSTALL_EXTRAS OFF" "CATCH_BUILD_TESTING OFF"
)

target_compile_options(Catch2 PUBLIC /EHsc)

list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/extras")

CPMAddPackage(NAME EnTT GITHUB_REPOSITORY skypjack/entt VERSION 3.16.0)

CPMAddPackage(
    NAME JoltPhysics
    GITHUB_REPOSITORY jrouwe/JoltPhysics
    VERSION 5.6.0
    SOURCE_SUBDIR Build
    OPTIONS "CROSS_PLATFORM_DETERMINISTIC ON"
            "CPP_EXCEPTIONS_ENABLED OFF"
            "CPP_RTTI_ENABLED OFF"
            "PROFILER_IN_DEBUG_AND_RELEASE OFF"
            "DEBUG_RENDERER_IN_DEBUG_AND_RELEASE OFF"
            "ENABLE_ALL_WARNINGS OFF"
            "OVERRIDE_CXX_FLAGS OFF"
            "ENABLE_OBJECT_STREAM OFF"
            "ENABLE_INSTALL OFF"
            "INTERPROCEDURAL_OPTIMIZATION OFF"
            "FLOATING_POINT_EXCEPTIONS_ENABLED OFF"
            "USE_SSE4_1 OFF"
            "USE_SSE4_2 OFF"
            "USE_AVX OFF"
            "USE_AVX2 OFF"
            "USE_LZCNT OFF"
            "USE_TZCNT OFF"
            "USE_F16C OFF"
            "USE_FMADD OFF"
            "USE_STATIC_MSVC_RUNTIME_LIBRARY OFF"
)

unison_apply_determinism(Jolt)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

CPMAddPackage(NAME enet GITHUB_REPOSITORY lsalzman/enet GIT_TAG v1.3.18)

unset(CMAKE_POLICY_VERSION_MINIMUM)

target_include_directories(enet PUBLIC "${enet_SOURCE_DIR}/include")

target_link_libraries(enet PUBLIC winmm ws2_32)

target_compile_options(enet PRIVATE /wd5287)

target_compile_definitions(enet PRIVATE _WINSOCK_DEPRECATED_NO_WARNINGS)

CPMAddPackage(NAME xxHash GITHUB_REPOSITORY Cyan4973/xxHash VERSION 0.8.4 DOWNLOAD_ONLY YES)

add_library(xxhash INTERFACE)

target_include_directories(xxhash INTERFACE "${xxHash_SOURCE_DIR}")

target_compile_definitions(xxhash INTERFACE XXH_INLINE_ALL)

CPMAddPackage(
    NAME tl-expected
    GITHUB_REPOSITORY TartanLlama/expected
    GIT_TAG v1.1.0
    OPTIONS "EXPECTED_BUILD_TESTS OFF"
)
