#include "luaapi_value.h"

using namespace lua::api;


static class _api_value_function: public I_value{
  public:
    void arith(void* istate, int op) override{lua_arith((lua_State*)istate, op);}
    int compare(void* istate, int idx1, int idx2, int op) override{return lua_compare((lua_State*)istate, idx1, idx2, op);}
    void concat(void* istate, int n) override{lua_concat((lua_State*)istate, n);}
    void createtable(void* istate, int narr, int nrec) override{lua_createtable((lua_State*)istate, narr, nrec);}
    int getfield(void* istate, int idx, const char* key) override{return lua_getfield((lua_State*)istate, idx, key);}
    int getglobal(void* istate, const char* name) override{return lua_getglobal((lua_State*)istate, name);}
    int geti(void* istate, int idx, lua_Integer i) override{return lua_geti((lua_State*)istate, idx, i);}
    int getmetatable(void* istate, int idx) override{return lua_getmetatable((lua_State*)istate, idx);}
    int gettable(void* istate, int idx) override{return lua_gettable((lua_State*)istate, idx);}
    int getuservalue(void* istate, int idx) override{return lua_getuservalue((lua_State*)istate, idx);}
    int isboolean(void* istate, int idx) override{return lua_isboolean((lua_State*)istate, idx);}
    int iscfunction(void* istate, int idx) override{return lua_iscfunction((lua_State*)istate, idx);}
    int isfunction(void* istate, int idx) override{return lua_isfunction((lua_State*)istate, idx);}
    int isinteger(void* istate, int idx) override{return lua_isinteger((lua_State*)istate, idx);}
    int islightuserdata(void* istate, int idx) override{return lua_islightuserdata((lua_State*)istate, idx);}
    int isnil(void* istate, int idx) override{return lua_isnil((lua_State*)istate, idx);}
    int isnone(void* istate, int idx) override{return lua_isnone((lua_State*)istate, idx);}
    int isnoneornil(void* istate, int idx) override{return lua_isnoneornil((lua_State*)istate, idx);}
    int isnumber(void* istate, int idx) override{return lua_isnumber((lua_State*)istate, idx);}
    int isstring(void* istate, int idx) override{return lua_isstring((lua_State*)istate, idx);}
    int istable(void* istate, int idx) override{return lua_istable((lua_State*)istate, idx);}
    int isthread(void* istate, int idx) override{return lua_isthread((lua_State*)istate, idx);}
    int isuserdata(void* istate, int idx) override{return lua_isuserdata((lua_State*)istate, idx);}
    int isyieldable(void* istate) override{return lua_isyieldable((lua_State*)istate);}
    void len(void* istate, int idx) override{lua_len((lua_State*)istate, idx);}
    void newtable(void* istate) override{return lua_newtable((lua_State*)istate);}
    void* newthread(void* istate) override{return lua_newthread((lua_State*)istate);}
    void* newuserdata(void* istate, size_t size) override{return lua_newuserdata((lua_State*)istate, size);}
    int next(void* istate, int idx) override{return lua_next((lua_State*)istate, idx);}
    void pushboolean(void* istate, int b) override{lua_pushboolean((lua_State*)istate, b);}
    void pushcclosure(void* istate, lua_CFunction fn, int n) override{lua_pushcclosure((lua_State*)istate, fn, n);}
    void pushcfunction(void* istate, lua_CFunction fn) override{lua_pushcfunction((lua_State*)istate, fn);}
    void pushglobaltable(void* istate) override{lua_pushglobaltable((lua_State*)istate);}
    void pushinteger(void* istate, lua_Integer n) override{lua_pushinteger((lua_State*)istate, n);}
    void pushlightuserdata(void* istate, void* p) override{lua_pushlightuserdata((lua_State*)istate, p);}
    const char* pushliteral(void* istate, const char* str) override{return lua_pushliteral((lua_State*)istate, str);}
    const char* pushlstring(void* istate, const char* str, size_t len) override{return lua_pushlstring((lua_State*)istate, str, len);}
    void pushnil(void* istate) override{lua_pushnil((lua_State*)istate);}
    void pushnumber(void* istate, lua_Number n) override{lua_pushnumber((lua_State*)istate, n);}
    const char* pushstring(void* istate, const char* str) override{return lua_pushstring((lua_State*)istate, str);}
    int pushthread(void* istate) override{return lua_pushthread((lua_State*)istate);}
    const char* pushvfstring(void* istate, const char* fmt, va_list argp) override{return lua_pushvfstring((lua_State*)istate, fmt, argp);}
    int rawequal(void* istate, int idx1, int idx2) override{return lua_rawequal((lua_State*)istate, idx1, idx2);}
    int rawget(void* istate, int idx) override{return lua_rawget((lua_State*)istate, idx);}
    int rawgeti(void* istate, int idx, lua_Integer n) override{return lua_rawgeti((lua_State*)istate, idx, n);}
    int rawgetp(void* istate, int idx, const void* p) override{return lua_rawgetp((lua_State*)istate, idx, p);}
    size_t rawlen(void* istate, int idx) override{return lua_rawlen((lua_State*)istate, idx);}
    void rawseti(void* istate, int idx, lua_Integer n) override{return lua_rawseti((lua_State*)istate, idx, n);}
    void rawsetp(void* istate, int idx, const void* p) override{return lua_rawsetp((lua_State*)istate, idx, p);}
    void setfield(void* istate, int idx, const char* key) override{lua_setfield((lua_State*)istate, idx, key);}
    void setglobal(void* istate, const char* name) override{lua_setglobal((lua_State*)istate, name);}
    void seti(void* istate, int idx, lua_Integer n) override{lua_seti((lua_State*)istate, idx, n);}
    void setmetatable(void* istate, int idx) override{lua_setmetatable((lua_State*)istate, idx);}
    void settable(void* istate, int idx) override{lua_settable((lua_State*)istate, idx);}
    void setuservalue(void* istate, int idx) override{lua_setuservalue((lua_State*)istate, idx);}
    size_t stringtonumber(void* istate, const char* str) override{return lua_stringtonumber((lua_State*)istate, str);}
    int toboolean(void* istate, int idx) override{return lua_toboolean((lua_State*)istate, idx);}
    lua_CFunction tocfunction(void* istate, int idx) override{return lua_tocfunction((lua_State*)istate, idx);}
    lua_Integer tointeger(void* istate, int idx) override{return lua_tointeger((lua_State*)istate, idx);}
    lua_Integer tointegerx(void* istate, int idx, int* isnum) override{return lua_tointegerx((lua_State*)istate, idx, isnum);}
    const char* tolstring(void* istate, int idx, size_t* len) override{return lua_tolstring((lua_State*)istate, idx, len);}
    lua_Number tonumber(void* istate, int idx) override{return lua_tonumber((lua_State*)istate, idx);}
    lua_Number tonumberx(void* istate, int idx, int* isnum) override{return lua_tonumberx((lua_State*)istate, idx, isnum);}
    const void* topointer(void* istate, int idx) override{return lua_topointer((lua_State*)istate, idx);}
    void* tothread(void* istate, int idx) override{return lua_tothread((lua_State*)istate, idx);}
    void* touserdata(void* istate, int idx) override{return lua_touserdata((lua_State*)istate, idx);}
    int type(void* istate, int idx) override{return lua_type((lua_State*)istate, idx);}
    const char* ttypename(void* istate, int type) override{return lua_typename((lua_State*)istate, type);}


    const char* pushfstring(void* istate, const char* fmt, ...) override{
      va_list _list;
      va_start(_list, fmt);
      pushvfstring(istate, fmt, _list);
      va_end(_list);
    }
};


static _api_value_function __api_def;

DLLEXPORT lua::api::I_value* CPPLUA_GET_API_VALUE_DEFINITION(){
  return &__api_def;
}