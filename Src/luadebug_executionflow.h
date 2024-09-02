#ifndef LUADEBUG_EXCUTIONFLOW
#define LUADEBUG_EXCUTIONFLOW

#include "I_debug_user.h"
#include "I_logger.h"
#include "library_linking.h"
#include "lua_includes.h"
#include "luadebug_hookhandler.h"

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

namespace lua::debug{
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

      virtual unsigned int get_function_layer() const = 0;
      virtual const char* get_function_name() const = 0;

      virtual int get_current_line() const = 0;
      virtual const char* get_current_file_path() const = 0;

      virtual bool set_function_name(int layer_idx, const char* name) = 0;
      
      virtual void block_execution() = 0;
      virtual void resume_execution() = 0;
      virtual void step_execution(step_type st) = 0;

      virtual bool currently_pausing() = 0;

#if (_WIN64) || (_WIN32)
      virtual void register_event_resuming(HANDLE hevent) = 0;
      virtual void remove_event_resuming(HANDLE hevent) = 0;

      virtual void register_event_pausing(HANDLE hevent) = 0;
      virtual void remove_event_pausing(HANDLE hevent) = 0;
#endif 

      virtual const lua_Debug* get_debug_data() const = 0;
  };


  // NOTE: This will block the running thread for lua runtime. Preferably to use multi threading for lua and main processing.
  //  lua_State shouldn't be destroyed in this class' lifetime
  class execution_flow: public I_execution_flow{
    private:
      struct _function_data{
        std::string name;
        bool is_tailcall;
      };

      hook_handler* _hook_handler;
      lua_State* _this_state;

      I_logger* _logger;


      bool _do_block = false;

      std::string _tmp_current_fname;

      std::mutex _execution_mutex;
      std::condition_variable _execution_block;
      bool _currently_executing = true;

      step_type _stepping_type = st_none;
      unsigned int _step_layer_check;

      int _current_line;
      std::string _current_file_path;
      std::vector<_function_data*> _function_layer;

#if (_WIN64) || (_WIN32)
      // events
      std::set<HANDLE> 
        _event_resuming,
        _event_pausing
      ;
#endif

      // check if current thread is not the running thread
      bool _check_running_thread();

      void _update_tmp_current_fname();

      void _block_execution();

      void _hookcb();

      static void _set_bind_obj(execution_flow* obj, lua_State* state);

      static void _hookcb_static(lua_State* state, void* cb_data);

    public:
      execution_flow(lua_State* state);
      ~execution_flow();

      static execution_flow* get_attached_obj(lua_State* state);

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

#if (_WIN64) || (_WIN32)
      void register_event_resuming(HANDLE hevent) override;
      void remove_event_resuming(HANDLE hevent) override;

      void register_event_pausing(HANDLE hevent) override;
      void remove_event_pausing(HANDLE hevent) override;
#endif

      const lua_Debug* get_debug_data() const override;

      void set_logger(I_logger* logger) override;
  };
}

#endif