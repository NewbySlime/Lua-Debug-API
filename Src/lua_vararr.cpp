#include "lua_vararr.h"

using namespace lua;


vararr::vararr(){
  
}


vararr::~vararr(){
  clear();
}


const lua::I_variant* vararr::get_var(int idx) const{
  if(idx < 0 || idx >= _data_arr.size())
    return NULL;

  return _data_arr[idx];
}


bool vararr::set_var(const I_variant* data, int idx){
  if(idx < 0 || idx >= _data_arr.size())
    return false;
  
  cpplua_delete_variant(_data_arr[idx]);
  _data_arr[idx] = cpplua_create_var_copy(data);
  return true;
}

void vararr::append_var(const I_variant* data){
  _data_arr.insert(_data_arr.end(), cpplua_create_var_copy(data));
}


void vararr::clear(){
  for(auto var: _data_arr)
    cpplua_delete_variant(var);
  
  _data_arr.clear();
}


std::size_t vararr::get_var_count() const{
  return _data_arr.size();
}