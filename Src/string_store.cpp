#include "string_store.h"


string_store::string_store(const char* cstr){
  data = cstr;
}

string_store::string_store(const std::string& str){
  data = str;
}


void string_store::append(const char* cstr){
  data.append(cstr);
}

void string_store::append(const char* cstr, std::size_t length){
  data.append(cstr, length);
}