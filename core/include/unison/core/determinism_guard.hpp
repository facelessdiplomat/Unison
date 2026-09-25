#pragma once

#if defined(_MSC_VER) && !defined(__clang__)
    #ifndef _M_FP_PRECISE
        #error "unison: deterministic code requires /fp:precise; fast, strict and except modes change results"
    #endif

    #ifdef _M_FP_CONTRACT
        #error "unison: deterministic code requires floating-point contraction to stay off"
    #endif
#elif defined(__clang__)
    #if defined(__FAST_MATH__) || __FINITE_MATH_ONLY__
        #error "unison: deterministic code requires IEEE arithmetic; -ffast-math and its parts change results"
    #endif

    #pragma STDC FP_CONTRACT OFF
#else
    #error "unison: deterministic code requires MSVC or clang"
#endif
