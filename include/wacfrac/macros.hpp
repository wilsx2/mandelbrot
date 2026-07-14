#if defined(__CUDACC__)
    #if !defined (WF_HD)
        #define WF_HD __host__ __device__
    #endif
    #if !defined (WF_STD)
        #define WF_STD cuda::std
    #endif
#else
    #if !defined (WF_HD)
        #define WF_HD
    #endif
    #if !defined (WF_STD)
        #define WF_STD std
    #endif
#endif
