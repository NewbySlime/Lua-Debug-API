#ifndef STRUTIL_HEADER
#define STRUTIL_HEADER

#include "string"

#ifdef USING_LUA_API
#include "luamemory_util.h"
#endif // USING_LUA_API


typedef bool (*character_filter)(char ch);

// Returns SIZE_MAX if no character found
int string_find_character(const char* str, character_filter filter);
// Returns SIZE_MAX if no character found
int string_find_character(const char* str, size_t strlen, character_filter filter);

template<typename... T_args> std::string format_str(const char* str, T_args... args){
#ifdef USING_LUA_API
  const dynamic_management* __dm = get_memory_manager();
#endif // USING_LUA_API

  int _str_len = snprintf(NULL, 0, str, args...);

#ifdef USING_LUA_API
  char* _c_str = (char*)__dm->malloc(_str_len+1);
#else
  char* _c_str = (char*)malloc(_str_len+1);
#endif // USING_LUA_API

  snprintf(_c_str, _str_len+1, str, args...);

  std::string _result = _c_str;

#ifdef USING_LUA_API
  __dm->free(_c_str);
#else
  free(_c_str);
#endif // USING_LUA_API

  return _result;  
}

#endif