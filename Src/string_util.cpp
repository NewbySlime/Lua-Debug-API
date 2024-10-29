#include "string_util.h"


// termination filter
typedef bool (*term_filter)(char ch);
bool _def_term_filter(char ch){ return ch == '\0'; }


// string_find_character
int _strfc(const char* str, size_t strlen, character_filter filter, term_filter _tfilter = NULL){
  int i;
  for(i = 0; i < strlen; i++){
    char ch = str[i];
    if(filter(ch))
      break;

    if(_tfilter && _tfilter(ch))
      return INT_MAX;
  }

  return i;
}


int string_find_character(const char* str, character_filter filter){
  return _strfc(str, SIZE_MAX, filter, _def_term_filter);
}

int string_find_character(const char* str, size_t strlen, character_filter filter){
  return _strfc(str, strlen, filter);
}