#include "string_store.h"


void string_store::append(const char* data){
  this->data.append(data);
}

void string_store::append(const char* data, size_t length){
  this->data.append(data, length);
}