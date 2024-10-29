#ifndef STRUTIL_HEADER
#define STRUTIL_HEADER

#include "memdynamic_management.h"
#include "string"


typedef bool (*character_filter)(char ch);

// Returns SIZE_MAX if no character found
int string_find_character(const char* str, character_filter filter);
// Returns SIZE_MAX if no character found
int string_find_character(const char* str, size_t strlen, character_filter filter);

template<typename... T_args> std::string format_str(const char* str, T_args... args){
#ifdef USING_MEMDYNAMIC_MANAGEMENT
  ::memory::I_dynamic_management* __dm = MEMDYNAMIC_MANAGEMENT_GET_STATIC_INSTANCE();
#endif // USING_MEMDYNAMIC_MANAGEMENT

  int _str_len = snprintf(NULL, 0, str, args...);
#ifdef USING_MEMDYNAMIC_MANAGEMENT
  char* _c_str = (char*)__dm->malloc(_str_len+1);
#else
  char* _c_str = (char*)malloc(_str_len+1);
#endif // USING_MEMDYNAMIC_MANAGEMENT
  snprintf(_c_str, _str_len+1, str, args...);
  std::string _result = _c_str;
#ifdef USING_MEMDYNAMIC_MANAGEMENT
  __dm->free(_c_str);
#else
  free(_c_str);
#endif // USING_MEMDYNAMIC_MANAGEMENT

  return _result;  
}

template<typename... T_args> std::string format_str_mem(const ::memory::I_dynamic_management* dynamic_man, const char* str, T_args... args){
  int _str_len = snprintf(NULL, 0, str, args...);
  char* _c_str = (char*)dynamic_man->malloc(_str_len+1, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  snprintf(_c_str, _str_len+1, str, args...);
  std::string _result = _c_str;
  dynamic_man->free(_c_str, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  return _result;  
}

#endif