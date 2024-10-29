#include "dllutil.h"
#include "luamemory_util.h"
#include "memory"
#include "Windows.h"

using namespace dynamic_library::util;
using namespace lua::memory;
using namespace ::memory;



I_dynamic_management* lua::memory::get_memory_manager(){
  return MEMDYNAMIC_MANAGEMENT_GET_STATIC_INSTANCE();
}


const memory_management_config* lua::memory::get_memory_manager_config(){
  return MEMDYNAMIC_MANAGEMENT_GET_STATIC_INSTANCE()->get_config();
}

void lua::memory::set_memory_manager_config(const memory_management_config* config){
  MEMDYNAMIC_MANAGEMENT_GET_STATIC_INSTANCE()->set_config(config);
}


static void* __lua_alloc(void* ud, void* ptr, size_t osize, size_t nsize){
  I_dynamic_management* __dm = MEMDYNAMIC_MANAGEMENT_GET_STATIC_INSTANCE();
  if(!ptr)
    return __dm->malloc(nsize, DYNAMIC_MANAGEMENT_DEBUG_DATA);

  if(!nsize){
    __dm->free(ptr, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    return NULL;
  }

  return __dm->realloc(ptr, nsize, DYNAMIC_MANAGEMENT_DEBUG_DATA);
}

lua_Alloc lua::memory::memory_manager_as_lua_alloc(){
  return __lua_alloc;
}