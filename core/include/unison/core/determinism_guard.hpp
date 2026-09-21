#pragma once

#ifndef _M_FP_PRECISE
    #error "unison: deterministic code requires /fp:precise; fast, strict and except modes change results"
#endif

#ifdef _M_FP_CONTRACT
    #error "unison: deterministic code requires floating-point contraction to stay off"
#endif
