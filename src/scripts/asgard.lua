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
    "asgard/extensions.lua"
  }
  for index, value in pairs(scripts) do
    loadscript(value)
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
  -- set_encoding("utf8")
  if read_orders(orders) ~= 0 then
    print("could not read " .. orders)
    return -1
  end

  plan_monsters()

  local nmrs = get_nmrs(1)
  if nmrs >= 80 then
    print("Shit. More than 80 factions with 1 NMR (" .. nmrs .. ")")
    write_summary()
    return -1
  end
  print (nmrs .. " Factions with 1 NMR")

  process_orders()

  -- post-turn updates:
  update_guards()
  update_scores()

  -- use newfactions file to place out new players
  autoseed(basepath .. "/newfactions", false)

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

