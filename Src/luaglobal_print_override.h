// see lbaselib.c for override global function
#ifndef LUASTREAM_PRINT_OVERRIDE_HEADER
#define LUASTREAM_PRINT_OVERRIDE_HEADER

#include "I_debug_user.h"
#include "library_linking.h"
#include "luaincludes.h"
#include "string_store.h"

#include "set"

#if (_WIN64) || (_WIN32)
#include "Windows.h"
#endif


namespace lua::global{
  class I_print_override: public I_debug_user{
    public:
      virtual ~I_print_override(){}

      virtual unsigned long read_n(char* buffer, unsigned long buffer_size) = 0;
      virtual unsigned long read_all(I_string_store* store) = 0;

      virtual unsigned long peek_n(char* buffer, unsigned long buffer_size) = 0;
      virtual unsigned long peek_all(I_string_store* store) = 0;

      virtual unsigned long available_bytes() = 0;

#if (_WIN64) || (_WIN32)
      virtual void register_event_read(HANDLE event) = 0;
      virtual void remove_event_read(HANDLE event) = 0;
#endif
  };


#ifdef LUA_CODE_EXISTS

  class print_override: public I_print_override{
    private:
#if (_WIN64) || (_WIN32)
      std::set<HANDLE> _event_read;

      HANDLE _pipe_write;
      HANDLE _pipe_read;

      HANDLE _class_mutex;
#endif

      I_logger* _logger;

      lua_State* _this_state;

      void _bind_global_function();

      static void _set_bind_obj(print_override* obj, lua_State* state);
      static int _on_print_static(lua_State* state);

    public:
      print_override(lua_State* state);
      ~print_override();

      static print_override* get_attached_object(lua_State* state);

      unsigned long read_n(char* buffer, unsigned long buffer_size) override;

      unsigned long read_all(I_string_store* store) override;
      std::string read_all();

      unsigned long peek_n(char* buffer, unsigned long buffer_size) override;

      unsigned long peek_all(I_string_store* store) override;
      std::string peek_all();

      unsigned long available_bytes() override;

#if (_WIN64) || (_WIN32)
      void register_event_read(HANDLE event) override;
      void remove_event_read(HANDLE event) override;
#endif

      void set_logger(I_logger* logger) override;
  };
#endif // LUA_CODE_EXISTS

}



// MARK: DLL functions

#define CPPLUA_CREATE_GLOBAL_PRINT_OVERRIDE cpplua_create_global_print_override
#define CPPLUA_CREATE_GLOBAL_PRINT_OVERRIDE_STR MACRO_TO_STR_EXP(CPPLUA_CREATE_GLOBAL_PRINT_OVERRIDE)

#define CPPLUA_DELETE_GLOBAL_PRINT_OVERRIDE cpplua_delete_global_print_override
#define CPPLUA_DELETE_GLOBAL_PRINT_OVERRIDE_STR MACRO_TO_STR_EXP(CPPLUA_DELETE_GLOBAL_PRINT_OVERRIDE)

typedef lua::global::I_print_override* (__stdcall *gpo_create_func)(void* state);
typedef void (__stdcall *gpo_delete_func)(lua::global::I_print_override* obj);

#ifdef LUA_CODE_EXISTS
DLLEXPORT lua::global::I_print_override* CPPLUA_CREATE_GLOBAL_PRINT_OVERRIDE(void* istate);
DLLEXPORT void CPPLUA_DELETE_GLOBAL_PRINT_OVERRIDE(lua::global::I_print_override* obj);
#endif // LUA_CODE_EXISTS

#endif