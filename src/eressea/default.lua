function process(orders)
  if read_game()~=0 then 
    print("could not read game")
    return -1
  end

  read_orders(orders)
  process_orders()

  write_passwords()
  write_reports()

  if write_game()~=0 then 
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
