#ifndef LIBRARY_LINKING_HEADER
#define LIBRARY_LINKING_HEADER

#if AS_RTLIB
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

#endif