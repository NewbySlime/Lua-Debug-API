DEBUG_OPTION_FLAG= -g -O0 -ggdb


NIL_VAR=

COMPILE_DEBUG_OPTIONS= 
CLUA_COMPILE_DEBUG_OPTIONS=
CPPLIB_COMPILE_DEBUG_OPTIONS=

CLUA_SOURCE_FOLDER= LuaSrc/*.c
CPPLIB_SOURCE_FOLDER= Src/*.cpp
CPPLIB_SOURCE_STATIC_DLIB_SCRIPTS= Src/lua_vararr.cpp Src/lua_variant.cpp Src/string_store.cpp Src/stdlogger.cpp Src/luatable_util.cpp

MAIN_PROGRAM_FILE= TestSrc/test_dll.cpp
OUTPUT_FILE= main
DLL_OUTPUT_FILE= CPPAPI.dll
STATIC_DLIB_OUTPUT_FILE = CPPAPI_static.lib

LINK_OPTION_FLAG= -static-libgcc -static-libstdc++

BASE_COMPILE_COMMAND= g++ -c

COMMAND_COMPILE_LUA= $(BASE_COMPILE_COMMAND) $(CLUA_SOURCE_FOLDER) $(CLUA_COMPILE_DEBUG_OPTIONS)
COMMAND_COMPILE_CPPLIB= $(BASE_COMPILE_COMMAND) $(CPPLIB_SOURCE_FOLDER) -std=c++17 $(CPPLIB_COMPILE_DEBUG_OPTIONS)
COMMAND_COMPILE_CPPLIB_STATIC_DLIB= $(BASE_COMPILE_COMMAND) $(CPPLIB_SOURCE_STATIC_DLIB_SCRIPTS) -std=c++17 $(CPPLIB_COMPILE_DEBUG_OPTIONS)

MISC_COMPILE_OPTION=


_add_debug_option:
	$(eval COMPILE_DEBUG_OPTIONS = $(DEBUG_OPTION_FLAG))

debug_mode: _add_debug_option
	$(eval CPPLIB_COMPILE_DEBUG_OPTIONS = $(DEBUG_OPTION_FLAG))

clua_debug_mode: _add_debug_option
	$(eval CLUA_COMPILE_DEBUG_OPTIONS = $(DEBUG_OPTION_FLAG))

do_clean:
	DEL /Q /F /S "*.o"

clua_compile:
	$(COMMAND_COMPILE_LUA)

cpplib_lua_compile:
	$(COMMAND_COMPILE_CPPLIB)

cpplib_lua_static_dlib_compile:
	$(COMMAND_COMPILE_CPPLIB_STATIC_DLIB)

skip_cpplib_compile:
	$(eval COMMAND_COMPILE_CPPLIB = $(NIL_VAR))
	$(eval CPPLIB_SOURCE_FOLDER = $(NIL_VAR))

skip_clua_compile:
	$(eval COMMAND_COMPILE_LUA = $(NIL_VAR))
	$(eval CLUA_SOURCE_FOLDER = $(NIL_VAR))

compile_main: 
	g++ $(MAIN_PROGRAM_FILE) $(LINK_OPTION_FLAG) ./*.o -o $(OUTPUT_FILE) -std=c++17 $(COMPILE_DEBUG_OPTIONS) $(MISC_COMPILE_OPTION)

link_dll:
	g++ $(LINK_OPTION_FLAG) ./*.o $(LINK_OPTION_FLAG) -shared -o $(DLL_OUTPUT_FILE)

mem_test:
	drmemory -logdir ./log -quiet -ignore_kernel -- $(OUTPUT_FILE)

dll_main: do_clean clua_compile cpplib_lua_compile link_dll
	DEL /Q /F /S "*.o"

static_dlib_main: do_clean clua_compile cpplib_lua_static_dlib_compile
	ld -r *.o -o $(STATIC_DLIB_OUTPUT_FILE)
	DEL /Q /F /S "*.o"

main: do_clean clua_compile cpplib_lua_compile compile_main
	DEL /Q /F /S "*.o"

minimal_main: static_dlib_main
	g++ $(MAIN_PROGRAM_FILE) $(LINK_OPTION_FLAG) $(STATIC_DLIB_OUTPUT_FILE) -o $(OUTPUT_FILE) -std=c++17 $(COMPILE_DEBUG_OPTIONS) $(MISC_COMPILE_OPTION)