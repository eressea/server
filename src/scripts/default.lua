function process(orders)
  file = "" .. get_turn()
  if read_game(file)~=0 then 
    print("could not read game")
    return -1
  end

  -- initialize starting equipment for new players
  -- probably not necessary, since mapper sets new players, not server
  add_equipment("conquesttoken", 1);
  add_equipment("wood", 30);
  add_equipment("stone", 30);
  add_equipment("money", 2000 + get_turn() * 10);

  read_orders(orders)
  process_orders()

  write_passwords()
  write_reports()

  print(turn .. " " .. get_turn())

  file = "" .. get_turn()
  if write_game(file)~=0 then 
    print("could not write game")
    return -1
  end
end


--
-- main body of script
--

-- orderfile: contains the name of the orders.
if orderfile==nil then
  print "you must specify an orderfile"
else
  process(orderfile)
end
