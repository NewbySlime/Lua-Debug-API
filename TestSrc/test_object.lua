print("test")

test = create_test()
print("hello")
print(test)

test:add_current_number(1)
print(test:get_current_number())
test:add_current_number(test:get_current_number() * 10)
print(test:get_current_number())
print("done")