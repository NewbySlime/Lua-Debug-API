#include "std_logger.h"

#include "stdio.h"


static std_logger __static_std_logger = std_logger();


void std_logger::print(std::string message) const{
  this->print(message.c_str());
}

void std_logger::print_warning(std::string message) const{
  this->print_warning(message.c_str());
}

void std_logger::print_error(std::string message) const{
  this->print_error(message.c_str());
}


void std_logger::print(const char* message) const{
  printf("[LOG] %s", message);
}

void std_logger::print_warning(const char* message) const{
  printf("[WRN] %s", message);
}

void std_logger::print_error(const char* message) const{
  printf("[ERR] %s", message);
}



I_logger* get_std_logger(){
  return &__static_std_logger;
}