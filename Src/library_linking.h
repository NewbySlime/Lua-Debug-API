#ifndef LIBRARY_LINKING_HEADER
#define LIBRARY_LINKING_HEADER

#if (_WIN64) || (_WIN32)

#define DLLEXPORT extern "C" __declspec(dllexport)

#elif (__linux)

#define DLLEXPORT __attribute__((visibility("default")))

#endif // #if (_WIN64) || (_WIN32)

#endif