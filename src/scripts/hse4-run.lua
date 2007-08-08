local locales = { "de", "en" }

function run_scripts()
  scripts = { 
    "default.lua",
    "spells.lua",
    "extensions.lua",
    "familiars.lua",
    "hse/portals.lua",
    "hse/stats.lua"
  }
  for index in scripts do
    local script = scriptpath .. "/" .. scripts[index]
    print("- loading " .. script)
    if pcall(dofile, script)==0 then
      print("Could not load " .. script)
    end
  end
end

function refresh_pool()
  for f in factions() do
    f:add_item("money", 50)
  end
end

function process(orders)
  file = "" .. get_turn()
  if read_game(file)~=0 then
    print("could not read game")
    return -1
  end

  -- run the turn:
  read_orders(orders)
  run_scripts()

  spawn_braineaters(0.25)
  plan_monsters()
  process_orders()
  
  refresh_pool()
  
  write_files(locales)

  file = "" .. get_turn()
  if write_game(file)~=0 then 
    print("could not write game")
    return -1
  end

  write_stats("grails.txt")
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

