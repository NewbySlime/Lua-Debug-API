// see lbaselib.c for override global function
#ifndef LUASTREAM_PRINT_OVERRIDE_HEADER
#define LUASTREAM_PRINT_OVERRIDE_HEADER

#include "defines.h"
#include "I_debug_user.h"
#include "library_linking.h"
#include "luaincludes.h"
#include "string_store.h"

#include "set"
#include "stdio.h"
#include "mutex"

#if (_WIN64) || (_WIN32)
#include "Windows.h"
#elif (__linux)
#include "sys/eventfd.h"
#endif


namespace lua::global{
  // [Thread-Safe]
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
#elif (__linux)
      virtual void register_event_read(int event_object) = 0;
      virtual void remove_event_read(int event_object) = 0;
#endif

      virtual void* get_lua_interface_state() const = 0;
  };


#ifdef LUA_CODE_EXISTS

  class print_override: public I_print_override{
    private:
#if (_WIN64) || (_WIN32)
      std::set<HANDLE> _event_read;
#elif (__linux)
      std::set<int> _event_read;
#endif

      bool _buffer_filled = false;
      size_t _start_iteration_index = 0;
      size_t _iteration_index = 0;
      size_t _buffer_len = 0;
      char* _buffer_data = NULL;

      std::recursive_mutex _class_mutex;
      std::recursive_mutex* _class_mutex_ptr;

      I_logger* _logger;

      lua_State* _this_state;

      void _bind_global_function();

      size_t _remaining_read_buffer();
      size_t _read_buffer(char* dest, size_t dest_len, bool is_peeking = false);
      size_t _write_buffer(const char* src, size_t src_len);

      void _lock_object() const;
      void _unlock_object() const;

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
#elif (__linux)
      void register_event_read(int event_object) override;
      void remove_event_read(int event_object) override;
#endif

      void* get_lua_interface_state() const override;

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