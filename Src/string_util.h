#ifndef STRUTIL_HEADER
#define STRUTIL_HEADER

#include "string"


typedef bool (*character_filter)(char ch);

// Returns SIZE_MAX if no character found
int string_find_character(const char* str, character_filter filter);
// Returns SIZE_MAX if no character found
int string_find_character(const char* str, size_t strlen, character_filter filter);

template<typename... T_args> std::string format_str(const char* str, T_args... args){
  int _str_len = snprintf(NULL, 0, str, args...);

  char* _c_str = (char*)malloc(_str_len+1);
  snprintf(_c_str, _str_len+1, str, args...);

  std::string _result = _c_str;
  free(_c_str);

  return _result;  
}

#endif