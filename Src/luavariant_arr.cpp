#include "luavariant_arr.h"

using namespace lua;


vararr::vararr(){}


vararr::~vararr(){
  clear();
}


const lua::I_variant* vararr::get_var(int idx) const{
  return get_self_var(idx);
}

const lua::variant* vararr::get_self_var(int idx) const{
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


void vararr::append(const I_vararr* arr){
  _data_arr.reserve(arr->get_var_count());

  for(int i = 0; i < arr->get_var_count(); i++)
    append_var(arr->get_var(i));
}

void vararr::copy_from(const I_vararr* arr){
  clear();
  append(arr);
}


void vararr::clear(){
  for(auto var: _data_arr)
    cpplua_delete_variant(var);
  
  _data_arr.clear();
}


std::size_t vararr::get_var_count() const{
  return _data_arr.size();
}