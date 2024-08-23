function do_calculate(n1, n2)
  print("calculating ", n1, n2)

  local _res1 = do_add(n1, n2)
  local _res2 = do_sub(n1, n2)

  return do_mult(_res1, _res2)
end

do_add();