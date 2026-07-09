#pragma once

#if !defined(__CUDACC__)
    #if !defined(__host__)
        #define __host__
    #endif
    #if !defined(__device__)
        #define __device__
    #endif
#endif
