#ifndef LUAAPI_VALUE_HEADER
#define LUAAPI_VALUE_HEADER

#include "luaincludes.h"
#include "library_linking.h"
#include "macro_helper.h"


namespace lua::api{
  class I_value{
    public:
      virtual void arith(void* istate, int op) = 0;
      virtual int compare(void* istate, int idx1, int idx2, int op) = 0;
      virtual void concat(void* istate, int n) = 0;
      virtual void createtable(void* istate, int narr, int nrec) = 0;
      virtual int getfield(void* istate, int idx, const char* key) = 0;
      virtual int getglobal(void* istate, const char* name) = 0;
      virtual int geti(void* istate, int idx, lua_Integer i) = 0;
      virtual int getmetatable(void* istate, int idx) = 0;
      virtual int gettable(void* istate, int idx) = 0;
      virtual int getuservalue(void* istate, int idx) = 0;
      virtual int isboolean(void* istate, int idx) = 0;
      virtual int iscfunction(void* istate, int idx) = 0;
      virtual int isfunction(void* istate, int idx) = 0;
      virtual int isinteger(void* istate, int idx) = 0;
      virtual int islightuserdata(void* istate, int idx) = 0;
      virtual int isnil(void* istate, int idx) = 0;
      virtual int isnone(void* istate, int idx) = 0;
      virtual int isnoneornil(void* istate, int idx) = 0;
      virtual int isnumber(void* istate, int idx) = 0;
      virtual int isstring(void* istate, int idx) = 0;
      virtual int istable(void* istate, int idx) = 0;
      virtual int isthread(void* istate, int idx) = 0;
      virtual int isuserdata(void* istate, int idx) = 0;
      virtual int isyieldable(void* istate) = 0;
      virtual void len(void* istate, int idx) = 0;
      virtual void newtable(void* istate) = 0;
      virtual void* newthread(void* istate) = 0;
      virtual void* newuserdata(void* istate, size_t size) = 0;
      virtual int next(void* istate, int idx) = 0;
      virtual void pushboolean(void* istate, int b) = 0;
      virtual void pushcclosure(void* istate, lua_CFunction fn, int n) = 0;
      virtual void pushcfunction(void* istate, lua_CFunction fn) = 0;
      virtual const char* pushfstring(void* istate, const char* fmt, ...) = 0;
      virtual void pushglobaltable(void* istate) = 0;
      virtual void pushinteger(void* istate, lua_Integer n) = 0;
      virtual void pushlightuserdata(void* istate, void* p) = 0;
      virtual const char* pushliteral(void* istate, const char* str) = 0;
      virtual const char* pushlstring(void* istate, const char* str, size_t len) = 0;
      virtual void pushnil(void* istate) = 0;
      virtual void pushnumber(void* istate, lua_Number n) = 0;
      virtual const char* pushstring(void* istate, const char* str) = 0;
      virtual int pushthread(void* istate) = 0;
      virtual const char* pushvfstring(void* istate, const char* fmt, va_list argp) = 0;
      virtual int rawequal(void* istate, int idx1, int idx2) = 0;
      virtual int rawget(void* istate, int idx) = 0;
      virtual int rawgeti(void* istate, int idx, lua_Integer n) = 0;
      virtual int rawgetp(void* istate, int idx, const void* p) = 0;
      virtual size_t rawlen(void* istate, int idx) = 0;
      virtual void rawseti(void* istate, int idx, lua_Integer n) = 0;
      virtual void rawsetp(void* istate, int idx, const void* p) = 0;
      virtual void lregister(void* istate, const char* name, lua_CFunction f) = 0;
      virtual void setfield(void* istate, int idx, const char* key) = 0;
      virtual void setglobal(void* istate, const char* name) = 0;
      virtual void seti(void* istate, int idx, lua_Integer n) = 0;
      virtual void setmetatable(void* istate, int idx) = 0;
      virtual void settable(void* istate, int idx) = 0;
      virtual void setuservalue(void* istate, int idx) = 0;
      virtual size_t stringtonumber(void* istate, const char* str) = 0;
      virtual int toboolean(void* istate, int idx) = 0;
      virtual lua_CFunction tocfunction(void* istate, int idx) = 0;
      virtual lua_Integer tointeger(void* istate, int idx) = 0;
      virtual lua_Integer tointegerx(void* istate, int idx, int* isnum) = 0;
      virtual const char* tolstring(void* istate, int idx, size_t* len) = 0;
      virtual lua_Number tonumber(void* istate, int idx) = 0;
      virtual lua_Number tonumberx(void* istate, int idx, int* isnum) = 0;
      virtual const void* topointer(void* istate, int idx) = 0;
      virtual void* tothread(void* istate, int idx) = 0;
      virtual void* touserdata(void* istate, int idx) = 0;
      virtual int type(void* istate, int idx) = 0;
      virtual const char* ttypename(void* istate, int type) = 0;
  };
}


#define CPPLUA_GET_API_VALUE_DEFINITION cpplua_get_api_value_definition
#define CPPLUA_GET_API_VALUE_DEFINITION_STR MACRO_TO_STR_EXP(CPPLUA_GET_API_VALUE_DEFINITION)

typedef lua::api::I_value* (__stdcall *get_api_value_func)();

#ifdef LUA_CODE_EXISTS
DLLEXPORT lua::api::I_value* CPPLUA_GET_API_VALUE_DEFINITION();
#endif // LUA_CODE_EXISTS

#endif