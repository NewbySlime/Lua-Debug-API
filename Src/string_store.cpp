#include "string_store.h"


void string_store::append(const char* cstr){
  data.append(cstr);
}

void string_store::append(const char* cstr, size_t length){
  data.append(cstr, length);
}