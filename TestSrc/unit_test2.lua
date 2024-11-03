# contains code (function) that can be imported

magic_number = 60265

table_data = {}


function get_table()
  return table_data
end

function change_table(key, number)
  table_data[key] = number
end


function get_magic_number()
  return magic_number
end