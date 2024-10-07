#ifndef LUA_VARARR_HEADER
#define LUA_VARARR_HEADER

#include "luavariant.h"
#include "vector"


// This code will be statically bind to the compilation file
// If a code returns an interface (I_xx) create a copy with using statically linked compilation function if the code that returns comes from dynamic library

namespace lua{
  class I_vararr{
    public:
      virtual ~I_vararr(){}

      // Function will not create a copy to return.
      virtual const lua::I_variant* get_var(int idx) const = 0;

      // Function will create a copy to store.
      virtual bool set_var(const lua::I_variant* data, int idx) = 0;
      // Function will create a copy to store.
      virtual void append_var(const lua::I_variant* data) = 0;

      virtual void append(const I_vararr* arr) = 0;
      virtual void copy_from(const I_vararr* arr) = 0;

      virtual void clear() = 0;

      virtual std::size_t get_var_count() const = 0;
  };


  class vararr: public I_vararr{
    private:
      std::vector<lua::variant*> _data_arr;

    public:
      vararr();
      ~vararr();

      const lua::I_variant* get_var(int idx) const override;
      const lua::variant* get_self_var(int idx) const;

      bool set_var(const lua::I_variant* data, int idx) override;
      void append_var(const lua::I_variant* data) override;

      void append(const I_vararr* arr) override;
      void copy_from(const I_vararr* arr) override;

      void clear() override;

      std::size_t get_var_count() const override;
  };
}

#endif