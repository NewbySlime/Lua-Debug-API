#ifndef MEMTRACKER_HEADER
#define MEMTRACKER_HEADER

#include "../../src/I_logger.h"
#include "../../src/library_linking.h"
#include "../../src/memdynamic_management.h"
#include "../macro_helper.h"
#include "Windows.h"

#include "chrono"
#include "map"


// A Hack: since dynamic_management does not support any function in its config to be complicated (i.e. using STL that might use new operator), memtracker should be compiled separately as a DLL. With this approach, memtracker can have its own new and delete function which is seperate than those used in dynamic_management.


class I_memtracker{
  public:
    virtual ~I_memtracker(){};

    virtual void check_memory_usage(I_logger* logger) const = 0;
    
    virtual void* allocate_memory(size_t size) = 0;
    virtual void deallocate_memory(void* address) = 0;
    virtual void* reallocate_memory(void* address, size_t new_size) = 0;
    virtual void lock_memory(bool acquire) = 0;
    virtual void set_memory_debug_data(const ::memory::memory_debug_data* data) = 0;

    virtual size_t get_maximal_usage() const = 0;
    virtual size_t get_current_usage() const = 0;

    virtual const ::memory::memory_management_config* get_configuration() const = 0;
};


class memtracker: public I_memtracker{
  private:
    struct _memory_data{
      void *address;
      size_t size;

      std::chrono::time_point<std::chrono::steady_clock> time_created;
      std::string function_created;
      long line_created;
    };

    std::map<void*, _memory_data*> _allocated_memory;

    ::memory::memory_management_config _management_config;
    
    size_t _maximum_usage = 0;
    size_t _current_usage = 0;

    std::chrono::time_point<std::chrono::steady_clock> _this_obj_creation_time;
    std::chrono::time_point<std::chrono::steady_clock> _maximal_reached_time;


    // relative to the creation of this object
    std::string _get_relative_time_str(const std::chrono::time_point<std::chrono::steady_clock>& time) const;

    ::memory::memory_debug_data _current_dbg_data;

    CRITICAL_SECTION _mem_mutex;

    void _add_usage(size_t mem_size);
    void _sub_usage(size_t mem_size);

    _memory_data* _create_new_metadata(void* address, size_t size);

  public:
    memtracker();
    ~memtracker();

    void check_memory_usage(I_logger* logger) const override;

    void* allocate_memory(size_t size) override;
    void deallocate_memory(void* address) override;
    void* reallocate_memory(void* address, size_t new_size) override;
    void lock_memory(bool acquire) override;
    void set_memory_debug_data(const ::memory::memory_debug_data* data) override;

    size_t get_maximal_usage() const override;
    size_t get_current_usage() const override;

    const ::memory::memory_management_config* get_configuration() const override;
};


#if (__linux)
// Just to make the compiler happy
#define __stdcall 
#endif


#define MEMTRACKER_CONSTRUCT memtracker_construct
#define MEMTRACKER_CONSTRUCT_STR MACRO_TO_STR_EXP(MEMTRACKER_CONSTRUCT)

#define MEMTRACKER_DECONSTRUCT memtracker_deconstruct
#define MEMTRACKER_DECONSTRUCT_STR MACRO_TO_STR_EXP(MEMTRACKER_DECONSTRUCT)

typedef I_memtracker* (__stdcall *memtracker_construct_func)();
typedef void (__stdcall *memtracker_deconstruct_func)(I_memtracker* tracker);


#endif