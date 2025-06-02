#ifndef DEFINES_HEADER
#define DEFINES_HEADER

#if (_WIN64) || (_WIN32)

#elif (__linux)

#ifndef max
#define max std::max
#endif

#ifndef min
#define min std::min
#endif

#define __stdcall 

#endif // #if (_WIN64) || (_WIN32)

#endif