#ifndef LUAMEMORY_UTIL_HEADER
#define LUAMEMORY_UTIL_HEADER

#include "luaincludes.h"
#include "memdynamic_management.h"


namespace lua::memory{
  ::memory::I_dynamic_management* get_memory_manager();

  const ::memory::memory_management_config* get_memory_manager_config();
  void set_memory_manager_config(const ::memory::memory_management_config* config);

  lua_Alloc memory_manager_as_lua_alloc();
}

#endif