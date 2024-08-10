DEBUG_OPTION_FLAG= -g -O0 -ggdb

NIL_VAR=

COMPILE_ADDED_SOURCE=
COMPILE_DEBUG_OPTIONS= 

CLUA_SOURCE_FOLDER= ../LuaSrc/*.c
CPPLIB_SOURCE_FOLDER= ../Src/*.cpp

COMMAND_COMPILE_LUA= g++ $(CLUA_SOURCE_FOLDER) -c
COMMAND_COMPILE_CPPLIB= g++ $(CPPLIB_SOURCE_FOLDER) -c -std=c++17


_add_debug_option:
	$(eval COMPILE_DEBUG_OPTIONS = $(DEBUG_OPTION_FLAG))

debug_mode: _add_debug_option
	$(eval COMMAND_COMPILE_CPPLIB = $(NIL_VAR))
	$(eval COMPILE_ADDED_SOURCE += $(CPPLIB_SOURCE_FOLDER))

clua_debug_mode: _add_debug_option
	$(eval COMMAND_COMPILE_CLUA = $(NIL_VAR))
	$(eval COMPILE_ADDED_SOURCE += $(CLUA_SOURCE_FOLDER))

do_clean:
	DEL /Q /F /S "*.o"

clua_compile:
	$(COMMAND_COMPILE_LUA)

cpplib_lua_compile:
	$(COMMAND_COMPILE_CPPLIB)

compile_main: 
	g++ $(COMPILE_ADDED_SOURCE) ../TestSrc/main.cpp ./*.o -o ../main $(COMPILE_DEBUG_OPTIONS)

compile_main_full_refresh: do_clean clua_compile cpplib_lua_compile compile_main
