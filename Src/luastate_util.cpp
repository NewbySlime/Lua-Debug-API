#if LUA_CODE_EXISTS

#include "luainternal_storage.h"
#include "luamemory_util.h"
#include "luastate_metadata.h"
#include "luastate_util.h"
#include "luathread_util.h"
#include "memdynamic_management.h"

#if (_WIN64) || (_WIN32)
#include "Windows.h"
#endif


using namespace lua; 
using namespace lua::internal;
using namespace lua::memory;
using namespace lua::state;
using namespace lua::thread;
using namespace ::memory;


static I_dynamic_management* __dm = get_memory_manager();


lua_State* lua::newstate(){
  lua_State* _result = lua_newstate(memory_manager_as_lua_alloc(), NULL);
  return _result;
}


void luastate_util_initstate(lua_State* lstate){
  // just in case
  deinitiate_metadata(lstate);
  initiate_metadata(lstate);
  
  require_state_mutex(lstate);
  
  // just in case
  reset_dependent_state(lstate);
  // trigger get dependent state
  require_dependent_state(lstate);
  
  initiate_internal_storage(lstate);
}

void luastate_util_deinitstate_start(lua_State* lstate){

}

void luastate_util_deinitstate_finish(lua_State* lstate){
  reset_dependent_state(lstate, true);
  deinitiate_metadata(lstate, true);
}


#endif