#include "luamemory_util.h"
#include "memory"

using namespace lua::memory;
using namespace ::memory;


static memory_management_config _default_config(malloc, free);
static dynamic_management _memdynamic_man(&_default_config);


const dynamic_management* lua::memory::get_memory_manager(){
  return &_memdynamic_man;
}


const memory_management_config* lua::memory::get_memory_manager_config(){
  return _memdynamic_man.get_config();
}

void lua::memory::set_memory_manager_config(const memory_management_config* config){
  _memdynamic_man.set_config(config);
}


static void* __lua_alloc(void* ud, void* ptr, size_t osize, size_t nsize){
  if(!ptr)
    return _memdynamic_man.malloc(nsize);

  if(!nsize){
    _memdynamic_man.free(ptr);
    return NULL;
  }

  return _memdynamic_man.realloc(ptr, nsize);
}

lua_Alloc lua::memory::memory_manager_as_lua_alloc(){
  return __lua_alloc;
}