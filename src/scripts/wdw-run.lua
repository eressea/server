function list_empty(list)
  -- trickfunktion, die rausfindet, ob es schon eine partei gibt.
  local _foo, _state, var_1 = list()
  local begin = _foo(_state, var_1)
  return begin == nil
end

function run_wdw()
  -- load 'wdw-start', if it exists. otherwise, load the latest turn, 
  -- and make a backup called 'wdw-start'.
  local file = "wdw-start"
  local alliance, position, faction
  if list_empty(factions) then
    if read_game(file)~=0 then 
      local turnfile = "" .. get_turn()
      if read_game(turnfile)~=0 then 
        print("could not read game")
        return -1
      end
      if write_game(file)~=0 then 
        print("could not write game")
        return -1
      end
    end
  end
  
  -- run the alliances setup
  if list_empty(alliances) then
    dofile("wdw-setup.lua")
  end
  -- -- run the turn (not yet)
  -- read_orders(orders)
  -- process_orders()

  -- siegbedingungen ausgeben
  dofile("wdw-standings.lua")
  
  -- write out the initial reports (no need to run a turn)
  write_passwords()
  write_reports()


  -- write the resulting file to 'wdw-setup'
  if write_game("wdw-setup")~=0 then 
    print("could not write game")
    return -1
  end
end


--
-- main body of the script
run_wdw()
