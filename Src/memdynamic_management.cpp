#include "memdynamic_management.h"

#include "memory"


using namespace memory;


dynamic_management::dynamic_management(const memory_management_config* config): _config(*config){
  set_config(config);
}


void* dynamic_management::malloc(size_t size) const{
  return _config.f_alloc(size);
}

void dynamic_management::free(void* obj) const{
  _config.f_dealloc(obj);
}


void* dynamic_management::realloc(void* target_obj, size_t size) const{
  void* _result = _config.f_alloc(size);
  memcpy(_result, target_obj, size);
  
  _config.f_dealloc(target_obj);

  return _result;
}


void dynamic_management::set_config(const memory_management_config* config){
  _config = *config;
}

const memory::memory_management_config* dynamic_management::get_config() const{
  return &_config;
}