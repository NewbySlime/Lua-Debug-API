local _test = {
  test = "this is a test"
}


local _testref = _test
_testref["another_test"] = "this is another test"
_testref[1] = "this is one"

print(string.format("modified in Lua: %s", _test["another_test"]))

set_to(_testref, "another_test", "another test modified in C++")
print(string.format("modified in C++: %s", _test["another_test"]))

set_to(_testref, "another_test", "welp it is modified.")
print(string.format("modified in C++: %s", _test["another_test"]))

local _newtable = create_copy(_testref);
set_to(_newtable, "another_test", "nothing in new table.")
print(string.format("table ref (pref): %s", _test["another_test"]))
print(string.format("table copy (new): %s", _newtable["another_test"]))