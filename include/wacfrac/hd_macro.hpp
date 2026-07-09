#if defined(__CUDACC__)
    #if !defined (WACFRAC_HD)
        #define WACFRAC_HD __host__ __device__
    #endif
#else
    #if !defined (WACFRAC_HD)
        #define WACFRAC_HD
    #endif
#endif 
