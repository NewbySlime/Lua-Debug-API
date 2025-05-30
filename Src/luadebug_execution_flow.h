#ifndef LUADEBUG_EXCUTION_FLOW_HEADER
#define LUADEBUG_EXCUTION_FLOW_HEADER

#include "I_debug_user.h"
#include "I_logger.h"
#include "library_linking.h"
#include "luaincludes.h"
#include "luadebug_hook_handler.h"
#include "macro_helper.h"

#include "condition_variable"
#include "mutex"
#include "set"
#include "string"
#include "thread"
#include "vector"

#if (_WIN64) || (_WIN32)
#include "Windows.h"
#endif


// This code will be statically bind to the compilation file
// If a code returns an interface (I_xx) create a copy with using statically linked compilation function if the code that returns comes from dynamic library
// NOTE: execution_flow no longer can be created instantly (in API context), as the setup is tedious. Use thread_control and thread_handle instead.

namespace lua::debug{
  // [Thread-Safe]
  class I_execution_flow: public I_debug_user{
    public:
      enum step_type{
        st_none,
        st_per_line,
        st_per_counts,
        st_in,
        st_out,
        st_over
      };

      virtual ~I_execution_flow(){};

      virtual void set_step_count(int count) = 0;
      virtual int get_step_count() const = 0;

      // Returns 0 if still executing.
      virtual unsigned int get_function_layer() const = 0;
      // Returns NULL if still executing.
      // NOTE: the address is only valid when not executing.
      virtual const char* get_function_name() const = 0;

      // Returns -1 if still executing.
      virtual int get_current_line() const = 0;
      // Returns 0 if still executing.
      virtual const char* get_current_file_path() const = 0;

      // Can return false if still executing.
      virtual bool set_function_name(int layer_idx, const char* name) = 0;
      
      virtual void block_execution() = 0;
      virtual void resume_execution() = 0;
      virtual void step_execution(step_type st) = 0;

      virtual bool currently_pausing() = 0;

      virtual void add_breakpoint(const char* file_name, int line) = 0;
      virtual bool remove_breakpoint(const char* file_name, int line) = 0;
      virtual bool clear_breakpoints(const char* file_name) = 0;
      virtual void clear_breakpoints() = 0;

#if (_WIN64) || (_WIN32)
      virtual void register_event_resuming(HANDLE hevent) = 0;
      virtual void remove_event_resuming(HANDLE hevent) = 0;

      virtual void register_event_pausing(HANDLE hevent) = 0;
      virtual void remove_event_pausing(HANDLE hevent) = 0;
#endif 

      // Returns NULL if still executing.
      virtual const lua_Debug* get_debug_data() const = 0;
  };


#ifdef LUA_CODE_EXISTS

  class hook_handler;

  // NOTE: This will block the running thread for lua runtime. Preferably to use multi threading for lua and main processing.
  //  lua_State shouldn't be destroyed in this class' lifetime
  class execution_flow: public I_execution_flow{
    private:
      struct _function_data{
        std::string name;
        bool is_tailcall;
      };

      struct _file_metadata{
        std::set<int> breakpointed_lines;
      };

      I_logger* _logger;

      I_hook_handler* _hook_handler;

      std::string _tmp_current_fname;
      volatile bool _currently_executing = true;
      volatile bool _do_block = false;

      volatile step_type _stepping_type = st_none;
      volatile unsigned int _step_layer_check;
      volatile int _step_line_check;

      volatile int _current_line;
      std::string _current_file_path;
      std::vector<_function_data*> _function_layer;

      std::map<std::string, _file_metadata*> _file_metadata_map;


#if (_WIN64) || (_WIN32)
      CRITICAL_SECTION _object_mutex;
      CRITICAL_SECTION* _object_mutex_ptr;
      HANDLE _execution_block_event;

      HANDLE _self_resume_event;
      HANDLE _self_pause_event;

      // events
      std::set<HANDLE> 
        _event_resuming,
        _event_pausing
      ;
#endif

      void _lock_object() const;
      void _unlock_object() const;

      void _update_tmp_current_fname();

      void _block_execution();

      void _hookcb();
      static void _hookcb_static(const lua::api::core* lc, void* cb_data);

    public:
      execution_flow(hook_handler* hook);
      ~execution_flow();

      void set_step_count(int count) override;
      int get_step_count() const override;

      unsigned int get_function_layer() const override;
      
      // Returned value is short lived (as it is stored in std::string in a std::vector that will be manipulated when function is called or returned)
      const char* get_function_name() const override;

      int get_current_line() const override;
      const char* get_current_file_path() const override;

      bool set_function_name(int layer_idx, const char* name) override;
      bool set_function_name(int layer_idx, const std::string& name);

      void block_execution() override;
      void resume_execution() override;
      void step_execution(step_type st) override;

      bool currently_pausing() override;

      void add_breakpoint(const char* file_name, int line) override;
      bool remove_breakpoint(const char* file_name, int line) override;
      bool clear_breakpoints(const char* file_name) override;
      void clear_breakpoints() override;

#if (_WIN64) || (_WIN32)
      void register_event_resuming(HANDLE hevent) override;
      void remove_event_resuming(HANDLE hevent) override;

      void register_event_pausing(HANDLE hevent) override;
      void remove_event_pausing(HANDLE hevent) override;
#endif

      const lua_Debug* get_debug_data() const override;

      void set_logger(I_logger* logger) override;
  };

#endif // LUA_CODE_EXISTS

}

#endif