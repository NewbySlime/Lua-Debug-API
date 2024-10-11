#ifndef MEMDYNAMIC_MANAGEMENT_HEADER
#define MEMDYNAMIC_MANAGEMENT_HEADER


namespace memory{
  typedef void* (*allocator_function)(size_t size);
  typedef void (*deallocator_function)(void* address);

  struct memory_management_config{
    memory_management_config(allocator_function fa, deallocator_function fd){
      f_alloc = fa;
      f_dealloc = fd;
    }

    allocator_function f_alloc;
    deallocator_function f_dealloc;
  };


  class I_dynamic_management{
    public:
      virtual void* malloc(size_t size) const = 0;
      virtual void free(void* obj) const = 0;

      virtual void* realloc(void* target_obj, size_t size) const = 0;

      virtual void set_config(const memory_management_config* const) = 0;
      virtual const memory_management_config* get_config() const = 0;
  };

  class dynamic_management: public I_dynamic_management{
    private:
      memory_management_config _config;

    public:
      dynamic_management(const memory_management_config* config);

      void* malloc(size_t size) const override;
      void free(void* obj) const override;

      void* realloc(void* target_obj, size_t size) const override;

      void set_config(const memory_management_config* const) override;
      const memory_management_config* get_config() const override;

      template<typename T_class, typename... T_args> T_class* new_class(T_args... args) const{
        T_class* _obj = (T_class*)_config.f_alloc(sizeof(T_class));
        *_obj = T_class(args...);
        return _obj;
      }

      template<typename T_class> void delete_class(T_class* obj) const{
        obj->~T_class();
        free(obj);
      }
  };
}

#endif