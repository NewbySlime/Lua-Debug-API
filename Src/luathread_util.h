#ifndef LUATHREAD_UTIL_HEADER
#define LUATHREAD_UTIL_HEADER

#include "luaincludes.h"


// Utilities for handling Multithreaded programs that uses Lua.
// Multithreading in a program that uses lua_State should also consider that each thread can only access a single lua_State.
// NOTE: Multithreading Lua code is not yet a feature as of now. Multithreading only allowed for main program that uses Lua API, not for running Lua code. Running thread only allowed to exist once running Lua code (handled by runtime_handler). 



namespace lua::thread{
  class I_mutex{
    public:
      virtual void lock() = 0;
      virtual void unlock() = 0;
  };

#ifdef LUA_CODE_EXISTS
  // Create a mutex or get the existing one.
  // Mutex will automatically deleted when lua_State is closed.
  I_mutex* require_state_mutex(lua_State* state);
  
  // Get a thread dependent state that is based on the calling thread. Will create a new lua_State if not exists for current calling thread. 
  lua_State* require_dependent_state(lua_State* state);

  // Enable the usage of thread dependent state. If disabled, getting a thread dependent will returns the original thread that the called supplied as parameter.
  void thread_dependent_enable(lua_State* lstate, bool enable);

  void lock_state(lua_State* lstate);
  void unlock_state(lua_State* lstate);
#endif // LUA_CODE_EXISTS
}



// Used by Lua

#ifdef LUA_CODE_EXISTS
void luathread_util_lock_state(lua_State* lstate);
void luathread_util_unlock_state(lua_State* lstate);

lua_State* luathread_util_get_tdependent(lua_State* lstate);

void luathread_util_enable_tdependent(lua_State* lstate);
void luathread_util_disable_tdependent(lua_State* lstate);
#endif // LUA_CODE_EXISTS


#endif