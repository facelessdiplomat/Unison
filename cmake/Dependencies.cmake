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
