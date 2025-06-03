#ifndef DEFINES_HEADER
#define DEFINES_HEADER

#if (_WIN64) || (_WIN32)

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#elif (__linux)

#define __stdcall 

#endif // #if (_WIN64) || (_WIN32)

#endif