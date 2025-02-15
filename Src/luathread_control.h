#ifndef LUATHREAD_CONTROL_HEADER
#define LUATHREAD_CONTROL_HEADER

#include "dllutil.h"
#include "error_util.h"
#include "I_debug_user.h"
#include "luaincludes.h"

#if (_WIN64) || (_WIN32)
#include "Windows.h"
#elif (__linux)
#include "pthread.h"
#endif

#include "map"
#include "memory"
#include "mutex"
#include "set"
#include "thread"


#if !((_WIN64) || (_WIN32))
#define INFINITE 0xFFFFFFFF
#endif


namespace lua{
  class I_variant;
  class runtime_handler;

  namespace api{
    struct core;
  }

  namespace debug{
    class I_execution_flow;
    class I_hook_handler;
  }

  // [Thread-Safe]
  class I_thread_handle{
    public:
      // Blocking until the thread is stopped.
      virtual void stop_running() = 0;
      virtual void signal_stop() = 0;

      // DEPRECATED
      // virtual bool wait_for_thread_stop(unsigned long wait_ms = INFINITE) = 0;

      // Called when the thread is signalled to stop.
      virtual bool is_stop_signal() const = 0;
      virtual bool is_stopped() const = 0;

      // DEPRECATED
      // virtual void pause() = 0;
      // DEPRECATED
      // virtual void resume() = 0;
      // DEPRECATED
      // virtual bool is_paused() = 0;

      virtual void join() = 0;
      virtual void try_join(unsigned long wait_ms) = 0;
      virtual bool joinable() = 0;
      virtual void detach() = 0; 
      
      virtual unsigned long get_thread_id() const = 0;

      virtual void* get_lua_state_interface() const = 0;
      virtual lua::debug::I_execution_flow* get_execution_flow_interface() const = 0;
      virtual lua::debug::I_hook_handler* get_hook_handler_interface() const = 0; 
  };

  // [Not Thread-Safe]
  class I_thread_handle_reference{
    public:
      virtual ~I_thread_handle_reference(){}

      virtual I_thread_handle* get_interface() const = 0;
  };


  // [Thread-Safe]
  class I_thread_control: public I_debug_user{
    public:
      typedef void (*execution_context)(void* data);

      virtual ~I_thread_control(){}

      // When using a looping (be it a poll or any loop that can lead to infinite loop), make sure to check is_stop_signal() as a way to stop the thread safely.
      // Returns the ID of the thread.
      virtual unsigned long run_execution(execution_context cb, void* cbdata) = 0;

      // Pointer returned from this function should be freed in free_thread_handle().
      // Might return NULL if the ID is not valid.
      virtual I_thread_handle_reference* get_thread_handle(unsigned long thread_id) = 0;
      virtual void free_thread_handle(I_thread_handle_reference* reference) = 0;
  };


  namespace debug{
    class execution_flow;
    class hook_handler;
  }

  class thread_control;
  class thread_handle: public I_thread_handle{
    friend thread_control;
    private:
      bool _stop_thread;
      lua_State* _current_tstate;

      lua::debug::execution_flow* _exec_flow;
      lua::debug::hook_handler* _hook;

      // Use kernel types, std::thread cannot be copied
#if (_WIN64) || (_WIN32)
      HANDLE _thread_handle;
#elif (__linux)
      pthread_t _thread_id;
#endif

      std::recursive_mutex _object_mutex;

      void _signal_stop();

      void _init_class(lua_State* lstate);

      void _hook_cb(lua_State* state);
      static void _hook_cb_static(const lua::api::core* lc, void* cbdata);

    public:
#if (_WIN64) || (_WIN32)
      thread_handle(HANDLE thread_handle, lua_State* lstate);
#elif (__linux)
      // TODO linux
#endif
      ~thread_handle();

      void stop_running() override;
      void signal_stop() override;

      bool is_stop_signal() const override;
      bool is_stopped() const override;

      void join() override;
      void try_join(unsigned long wait_ms) override;
      bool joinable() override;
      void detach() override;

      unsigned long get_thread_id() const override;

      void* get_lua_state_interface() const override;
      lua_State* get_lua_state() const;
      
      lua::debug::I_execution_flow* get_execution_flow_interface() const override;
      lua::debug::I_hook_handler* get_hook_handler_interface() const override;
  };

  class thread_handle_reference: public I_thread_handle_reference{
    public:
      typedef void (*handle_destructor)(thread_handle*);
    
    private:
      std::shared_ptr<thread_handle> _thread_ptr;

    public:
      thread_handle_reference(const thread_handle_reference* ref);
      thread_handle_reference(thread_handle* handle, handle_destructor destructor);
      ~thread_handle_reference();

      thread_handle* get() const;
      I_thread_handle* get_interface() const override;
  };


  class thread_control: public I_thread_control{
    private:
      struct _t_entry_point_data{
        execution_context cb;
        void* cbdata;
        lua_State* lstate;

        thread_control* _this;

#if (_WIN64) || (_WIN32)
        HANDLE thread_handle;
#elif (__linux)
        pthread_t thread_handle;
#endif

        std::condition_variable init_cond;
        std::mutex init_mutex;
      };

      I_logger* _logger;

      std::map<unsigned long, thread_handle_reference*> _thread_data;
      lua::runtime_handler* _rh;

      lua_State* _state;

      std::recursive_mutex _object_mutex;
      std::recursive_mutex* _object_mutex_ptr;

#if (_WIN64) || (_WIN32)
      static DWORD WINAPI __thread_entry_point(LPVOID data);
#elif (__linux)
      static void* __thread_entry_point(void* data);
#endif

      static void __thread_entry_point_generalize(_t_entry_point_data* data);

      void _stop_all_execution();

    public:
      thread_control(lua::runtime_handler* handler);
      ~thread_control();

      // This will create another thread specifically for running Lua code. If user code (that runs in this thread) uses loops, make sure to check is_thread_stopping() because the thread does not force stopped when signaled to stop.
      // Also after calling Lua functions, make sure to check if the thread is being stopped. Then returns safely. This way, a dynamic memory and a C++ object can safely deinitiated. Do not try to read the returned stuff, as this object always throw an error in Lua code execution when stopping.
      // The function returns the thread ID.
      unsigned long run_execution(execution_context cb, void* cbdata) override;

      I_thread_handle_reference* get_thread_handle(unsigned long thread_id) override;
      void free_thread_handle(I_thread_handle_reference* reference) override;

      void set_logger(I_logger* logger) override;
  };


  // Returns thread object based on current calling thread.
  // Might be NULL if thread does not created inside thread_control.
  thread_handle* get_thread_handle();
}

#endif