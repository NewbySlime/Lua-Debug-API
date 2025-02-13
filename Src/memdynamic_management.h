#ifndef MEMDYNAMIC_MANAGEMENT_HEADER
#define MEMDYNAMIC_MANAGEMENT_HEADER

#include "defines.h"
#include "library_linking.h"
#include "macro_helper.h"
#include "stddef.h"

// NOTE: dynamic_management overloads C++ new and delete global function
// NOTE: dynamic_management does not handle memory synchronzation, it is up to the custom memory synchronization

#ifdef DEBUG_BUILD
#define DYNAMIC_MANAGEMENT_DEBUG_DATA ::memory::memory_debug_data(__func__, __LINE__)
#else
#define DYNAMIC_MANAGEMENT_DEBUG_DATA ::memory::memory_debug_data()
#endif


namespace memory{
  struct memory_debug_data;

  typedef void* (*allocator_function)(size_t size, void* f_data);
  typedef void (*deallocator_function)(void* address, void* f_data);
  typedef void* (*reallocator_function)(void* address, size_t new_size, void* f_data);
  // [OPTIONAL] if acquire, lock the memory, if not, release the memory.
  typedef void (*synchronization_function)(bool acquire, void* f_data);
  // [OPTIONAL] sets the debug data about the caller.
  typedef void (*memory_debug_set_function)(const memory_debug_data* dbg_data, void* f_data);

  struct memory_debug_data{
    memory_debug_data(const char* fname = NULL, size_t line = 0){
      function_name = fname;
      code_line = line;
    }

    const char* function_name; // created at which function
    size_t code_line; // created at which line in the file
  };

  // NOTE: If synchronization function is included, before calling one of the function in this struct, it will do synchronization first.
  // WARN:
  //  - Functions in the configs should do minimal as possible.
  //  - Do not do any C++ dynamic memory management while still calling the functions included in the config. Doing so might introduce recursive calling (which might break the synchronization).
  struct memory_management_config{
    memory_management_config(allocator_function fa, deallocator_function fd, reallocator_function fr, synchronization_function fsync, memory_debug_set_function fdbg, void* data = NULL){
      f_data = data;
      f_alloc = fa;
      f_dealloc = fd;
      f_realloc = fr;
      
      f_sync = fsync;
      f_dbg = fdbg;
    }

    void* f_data = NULL;
    allocator_function f_alloc;
    deallocator_function f_dealloc;
    reallocator_function f_realloc;

    // if NULL, it will be ignored
    synchronization_function f_sync = NULL;
    // if NULL, it will be ignored
    memory_debug_set_function f_dbg = NULL;
  };

  // Used for handling nested new/delete calls.
  struct nested_class_record{
    const memory_debug_data* debug_data;
    nested_class_record* tail;
  };

  // [Thread-Safe, but based on the configuration]
  class I_dynamic_management{
    protected:
      virtual void _add_nested_record(nested_class_record* record) const = 0;
      virtual void _remove_nested_record() const = 0;

    public:
      virtual void* malloc(size_t size, const memory_debug_data& dbg_data) const = 0;
      virtual void* malloc(size_t size, const memory_debug_data* dbg_data) const = 0;
      virtual void* malloc(size_t size) const = 0;
      virtual void free(void* obj, const memory_debug_data& dbg_data) const = 0;
      virtual void free(void* obj, const memory_debug_data* dbg_data) const = 0;
      virtual void free(void* obj) const = 0;

      virtual void* realloc(void* target_obj, size_t size, const memory_debug_data& dbg_data) const = 0;
      virtual void* realloc(void* target_obj, size_t size, const memory_debug_data* dbg_data) const = 0;
      virtual void* realloc(void* target_obj, size_t size) const = 0;

      virtual void set_config(const memory_management_config* config) const = 0;
      virtual void reset_config() const = 0;
      virtual const memory_management_config* get_config() const = 0;



      template<typename T_class, typename... T_args> T_class* new_class_dbg(const memory_debug_data* dbg_data, T_args... args) const{
        nested_class_record _record;
          _record.debug_data = dbg_data;

        this->_add_nested_record(&_record);
        T_class* _result = new T_class(args...);
        this->_remove_nested_record();

        return _result;
      }

      template<typename T_class, typename... T_args> T_class* new_class_dbg(const memory_debug_data& dbg_data, T_args... args) const{
        return new_class_dbg<T_class>(&dbg_data, args...);
      }

      template<typename T_class, typename... T_args> T_class* new_class(T_args... args) const{
        return new_class_dbg<T_class>(NULL, args...);
      }


      template<typename T_class> void delete_class_dbg(T_class* obj, const memory_debug_data* dbg_data) const{
        nested_class_record _record;
          _record.debug_data = dbg_data;

        this->_add_nested_record(&_record);
        delete obj;
        this->_remove_nested_record();
      }

      template<typename T_class> void delete_class_dbg(T_class* obj, const memory_debug_data& dbg_data) const{
        delete_class_dbg<T_class>(obj, &dbg_data);
      }

      template <typename T_class> void delete_class(T_class* obj){
        delete_class_dbg<T_class>(obj, NULL);
      }
  };
}


#define MEMDYNAMIC_MANAGEMENT_GET_STATIC_INSTANCE memdynamic_management_get_static_instance
#define MEMDYNAMIC_MANAGEMENT_GET_STATIC_INSTANCE_STR MACRO_TO_STR_EXP(MEMDYNAMIC_MANAGEMENT_GET_STATIC_INSTANCE_STR)

typedef ::memory::I_dynamic_management* (__stdcall *get_memdynamic_management_static_instance)();

#ifdef USING_MEMDYNAMIC_MANAGEMENT
DLLEXPORT ::memory::I_dynamic_management* MEMDYNAMIC_MANAGEMENT_GET_STATIC_INSTANCE();
#endif // USING_MEMDYNAMIC_MANAGEMENT

#endif