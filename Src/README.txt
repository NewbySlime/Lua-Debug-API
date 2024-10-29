Prefered file naming convention in this code:
  First word (before seperator) is the primary context (of what kind of code that it includes), Second word and after (after seperator) is more detailed context (this code only includes a certain code).
  If the first word of the file name is 3 or less in character length, don't use seperator (such as Underscores or Spaces). Unless it is an abbreviation (such I_ for interface) or a need to seperate between primary context (only in less than or equal to 3 characters length) and detailed context.

Prefered variable/class/struct name declaration:
  C++: Use snake case, unless for abbreviation. For macros, use upper snake case.

When to develop using lua::api::core and lua_State:
  Preferably, use lua_State for every part of the library when it is only referenced and used inside the shared library. But when a part of the library also included both in static and dynamic library, use abstraction context such as lua::api::core. For example, usage in lua::variant.
  Libraries or any I_object should use lua::api::core due to a possibility of changes in Lua version. Technically, I_object is a part of object_var. For dependecies purposes, it is encouraged that the library objects uses lua::api::core.

Notes when developing the API:
  Looping inside thread created specifically for running Lua code, should check the thread if the thread is signalled to stop.
  

Notes when using dynamic memory:
  At all times, use dynamic_management used by Lua CPP API.