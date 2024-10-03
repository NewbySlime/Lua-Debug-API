Prefered file naming convention in this code:
  If the first word of the file name is 3 or less in character length, don't use seperator (such as Underscores or Spaces). Unless it is an abbreviation (such I_ for interface).

Prefered variable/class/struct name declaration:
  C++: Use snake case, unless for abbreviation. For macros, use upper snake case.

When to develop using lua::api::core and lua_State:
  Preferably, use lua_State for every part of the library when it is only referenced and used inside the shared library. But when a part of the library also included both in static and dynamic library, use abstraction context such as lua::api::core. For example, usage in lua::variant.