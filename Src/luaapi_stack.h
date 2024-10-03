#ifndef LUAAPI_STACK_HEADER
#define LUAAPI_STACK_HEADER

#include "luaincludes.h"
#include "luavariant.h"
#include "library_linking.h"
#include "macro_helper.h"


namespace lua::api{
  class I_stack{
    public:
      virtual int absindex(void* istate, int idx) = 0;
      virtual int checkstack(void* istate, int n) = 0;
      // If already using top indexing, will be skipped.
      virtual int convbottom2top(void* istate, int idx) = 0;
      // If already using bottom indexing, will be skipped.
      virtual int convtop2bottom(void* istate, int idx) = 0;
      virtual void copy(void* istate, int fromidx, int toidx) = 0;
      virtual int gettop(void* istate) = 0;
      virtual void insert(void* istate, int idx) = 0;
      virtual void pop(void* istate, int n) = 0;
      virtual void pushvalue(void* istate, int objidx) = 0;
      virtual void remove(void* istate, int idx) = 0;
      virtual void replace(void* istate, int idx) = 0;
      virtual void rotate(void* istate, int idx, int n) = 0;
      virtual void settop(void* istate, int idx) = 0;
      virtual void xmove(void* ifrom, void* ito, int n) = 0;
  };
}


#define CPPLUA_GET_API_STACK_DEFINITION cpplua_get_api_stack_definition
#define CPPLUA_GET_API_STACK_DEFINITION_STR MACRO_TO_STR_EXP(CPPLUA_GET_API_STACK_DEFINITION)

typedef lua::api::I_stack* (__stdcall *get_api_stack_func)();

#ifdef LUA_CODE_EXISTS
DLLEXPORT lua::api::I_stack* CPPLUA_GET_API_STACK_DEFINITION();
#endif // LUA_CODE_EXISTS

#endif