#pragma once
#if defined(__CUDACC__) && defined(_WIN32)
#include <crtdbg.h>
#undef _CrtDbgReport
#define _CrtDbgReport(...) ((int)0)
#endif
