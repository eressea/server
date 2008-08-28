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

function change_locales()
  -- local localechange = { }
  local localechange = { de = { "bb" }, en = { "rtph" } }
  
  for loc, flist in pairs(localechange) do
    for index, name in pairs(flist) do
      f = get_faction(atoi36(name))
      if f ~= nil then
        f.locale = loc
        print("LOCALECHANGE ", f, loc)
      end
    end
  end
end

function load_scripts()
  scripts = { 
    "spells.lua",
    "extensions.lua",
    "familiars.lua",
    "eressea/eternath.lua",
    "eressea/wedding-jadee.lua", 
    "eressea/ponnuki.lua",
    "eressea/items.lua",
    "eressea/10years.lua",
    "eressea/xmas2004.lua",
    "eressea/xmas2005.lua",
    "eressea/xmas2006.lua",
    "eressea/embassy.lua",
    "eressea/tunnels.lua",
    "eressea/ents.lua"
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

  -- create new monsters:
  spawn_dragons()
  spawn_undead()
  spawn_braineaters(0.25)
  spawn_ents()

  -- post-turn updates:
  update_xmas2006()
  update_embassies()
  update_guards()
  update_scores()

  change_locales()

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

