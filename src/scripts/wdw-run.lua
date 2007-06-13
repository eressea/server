function process(orders)
  -- read game and orders
  local turnfile = "" .. get_turn()
  if read_game(turnfile)~=0 then 
    print("could not read game")
    return -1
  end
  init_summary()

  -- read the orders file
  read_orders(orders)

  -- set up the sphinx
  -- init_sphinxhints()
  update_phoenix()
  sphinx_handler()
  sphinx_weekly()

  -- run the turn:

  plan_monsters()
  process_orders()

  -- create new monsters:
  spawn_dragons()
  spawn_undead()
  -- (no more braineaters) spawn_braineaters(0.25)

  -- post-turn updates:
  update_guards()
  update_scores()

  -- write out the initial reports (no need to run a turn)
  write_passwords()
  write_reports()
  write_emails()
  write_summary()

  -- siegbedingungen ausgeben
  write_standings()

  -- save the game
  outfile = "" .. get_turn()
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
  "spells.lua",
  "extensions.lua",
  "familiars.lua",
  "write_emails.lua",
  "wdw/sphinx.lua",
  "wdw/phoenix.lua",
  "wdw/standings.lua"
}

-- orderfile: contains the name of the orders.
if orderfile==nil then
  print "you must specify an orderfile"
else
  local name
  local index
  for index, name in pairs(scripts) do
    local script = scriptpath .. "/" .. name
    print("- loading " .. script)
    if pcall(dofile, script)==0 then
      print("Could not load " .. script)
    end
  end
  process(orderfile)
end
