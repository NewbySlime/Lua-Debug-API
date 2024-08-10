#include "stdlogger.h"

#include "stdio.h"


static stdlogger __static_std_logger = stdlogger();


void stdlogger::print(std::string message) const{
  this->print(message.c_str());
}

void stdlogger::print_warning(std::string message) const{
  this->print_warning(message.c_str());
}

void stdlogger::print_error(std::string message) const{
  this->print_error(message.c_str());
}


void stdlogger::print(const char* message) const{
  printf("[LOG] %s", message);
}

void stdlogger::print_warning(const char* message) const{
  printf("[WRN] %s", message);
}

void stdlogger::print_error(const char* message) const{
  printf("[ERR] %s", message);
}



I_logger* get_stdlogger(){
  return &__static_std_logger;
}