function process(orders)
  -- read game and orders
  local turnfile = "" .. get_turn()
  if read_game(turnfile)~=0 then 
    print("could not read game")
    return -1
  end

  -- read the orders file
  read_orders(orders)

  -- set up the sphinx
  -- init_sphinxhints()
  sphinx_handler()
  sphinx_weekly()

  -- run the turn:
  plan_monsters()
  process_orders()
  outfile = "" .. get_turn()

  -- write out the initial reports (no need to run a turn)
  write_passwords()
  write_reports()

  -- siegbedingungen ausgeben
  write_standings()

  -- save the game
  if write_game(outfile)~=0 then 
    print("could not write game")
    return -1
  end
end
	
--
-- main body of script
--

print("- Running wdw-run.lua")

scripts = { 
  "wdw-sphinx.lua",
  "wdw-standings.lua"
}

-- orderfile: contains the name of the orders.
if orderfile==nil then
  print "you must specify an orderfile"
else
  for index in scripts do
    local script = scriptpath .. "/" .. scripts[index]
    print("- loading " .. script)
    if pcall(dofile, script)==0 then
      print("Could not load " .. script)
    end
  end
  process(orderfile)
end
