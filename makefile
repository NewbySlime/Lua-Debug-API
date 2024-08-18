DEBUG_OPTION_FLAG= -g -O0 -ggdb


NIL_VAR=

COMPILE_ADDED_SOURCE=
COMPILE_DEBUG_OPTIONS= 

CLUA_SOURCE_FOLDER= LuaSrc/*.c
CPPLIB_SOURCE_FOLDER= Src/*.cpp

MAIN_PROGRAM_FILE= TestSrc/DebuggerSrc/main.cpp
OUTPUT_FILE= main

LINK_OPTION_FLAG= -static-libgcc -static-libstdc++

BASE_COMPILE_COMMAND= g++ -c

COMMAND_COMPILE_LUA= $(BASE_COMPILE_COMMAND) $(CLUA_SOURCE_FOLDER)
COMMAND_COMPILE_CPPLIB= $(BASE_COMPILE_COMMAND) $(CPPLIB_SOURCE_FOLDER) -std=c++17

MISC_COMPILE_OPTION=


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
	g++ $(MAIN_PROGRAM_FILE) $(COMPILE_ADDED_SOURCE) $(LINK_OPTION_FLAG) ./*.o -o $(OUTPUT_FILE) -std=c++17 $(COMPILE_DEBUG_OPTIONS) $(MISC_COMPILE_OPTION)

mem_test:
	drmemory -logdir ./log -quiet -ignore_kernel -- $(OUTPUT_FILE)

main: do_clean clua_compile cpplib_lua_compile compile_main
	DEL /Q /F /S "*.o"