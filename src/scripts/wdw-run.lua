function run_wdw()
  local turnfile = "" .. get_turn()
  if read_game(turnfile)~=0 then 
    print("could not read game")
    return -1
  end
  
  -- run the turn (not yet)
  read_orders(orderfile)
  process_orders()
  outfile = "" .. get_turn()

  -- siegbedingungen ausgeben
  dofile("wdw-standings.lua")
  
  -- write out the initial reports (no need to run a turn)
  write_passwords()
  write_reports()


  -- write the resulting file to 'wdw-setup'
  if write_game(outfile)~=0 then 
    print("could not write game")
    return -1
  end
end


--
-- main body of the script
run_wdw()
