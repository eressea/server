-- the locales that this gameworld supports.
local locales = { "de", "en" }

function loadscript(name)
  local script = scriptpath .. "/" .. name
  print("- loading " .. script)
  if pcall(dofile, script)==0 then
    print("Could not load " .. script)
  end
end

loadscript("default.lua")

function load_scripts()
  scripts = { 
    "spells.lua",
    "extensions.lua",
    "familiars.lua",
    "hse/portals.lua",
    "hse/stats.lua"
  }
  for index, value in pairs(scripts) do
    loadscript(value)
  end
end

function refresh_pool()
  for f in factions() do
    f:add_item("money", 50)
  end
end        
        
function process(orders)
  -- initialize starting equipment for new players
  if open_game(get_turn())~=0 then
    print("could not read game")
    return -1
  end
  init_summary()

  -- kill multi-players (external script)
  -- loadscript("eressea/multis.lua")

  -- run the turn:
  if read_orders(orders) ~= 0 then
    print("could not read " .. orders)
    return -1
  end

  plan_monsters()
  process_orders()

  -- create new monsters:
  spawn_braineaters(0.25)
  
  refresh_pool()
  write_files(locales)

  file = "" .. get_turn() .. ".dat"
  if write_game(file, "binary")~=0 then
    print("could not write game")
    return -1
  end
end

--
-- main body of script
--

-- orderfile: contains the name of the orders.
load_scripts()
if orderfile==nil then
  print "you must specify an orderfile"
else
  process(orderfile)
end

