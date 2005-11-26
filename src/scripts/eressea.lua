function loadscript(name)
  local script = scriptpath .. "/" .. name
  print("- loading " .. script)
  if pcall(dofile, script)==0 then
    print("Could not load " .. script)
  end
end

function change_locales()
  -- local localechange = { }
  local localechange = { de = { "2xwr" } }
  
  for loc, flist in localechange do
    for index, name in flist do
      f = get_faction(atoi36(name))
      if f ~= nil then
        f.locale = loc
        print("LOCALECHANGE ", f, loc)
      end
    end
  end
end

function run_scripts()
  scripts = { 
    "spells.lua",
    "extensions.lua",
    "eressea/eternath.lua",
    "eressea/wedding-jadee.lua", 
    "eressea/ponnuki.lua"
  }
  for index in scripts do
    loadscript(scripts[index])
  end
end

function write_emails()
  local locales = { "de", "en" }
  local files = {}
  local key
  for key in locales do
    local locale = locales[key]
    files[locale] = io.open(basepath .. "/emails." .. locale, "w")
  end

  local faction
  for faction in factions() do
    -- print(faction.id .. " - " .. faction.locale)
    files[faction.locale]:write(faction.email .. "\n")
  end

  for key in files do
    files[key]:close()
  end
end

function process(orders)
  -- initialize starting equipment for new players
  equipment_setitem("new_faction", "conquesttoken", "1");
  equipment_setitem("new_faction", "log", "30");
  equipment_setitem("new_faction", "stone", "30");
  equipment_setitem("new_faction", "money", "4200");

  file = "" .. get_turn()
  if read_game(file)~=0 then
    print("could not read game")
    return -1
  end
  init_summary()

  -- kill multi-players (external script)
  loadscript("eressea/multis.lua")

  -- run the turn:
  if read_orders(orders) ~= 0 then
    print("could not read " .. orders)
    return -1
  end
  run_scripts()

  -- create new monsters:
  spawn_dragons()
  spawn_undead()
  spawn_braineaters(0.25)

  plan_monsters()
  process_orders()

  -- post-turn updates:
  update_guards()
  update_scores()

  change_locales()
  
  -- use newfactions file to place out new players
  autoseed(basepath .. "/newfactions", false)

  write_passwords()
  write_reports()
  write_emails()
  write_summary()

  file = "" .. get_turn()
  if write_game(file)~=0 then 
    print("could not write game")
    return -1
  end
end

-- demo: how to add a special familiars-constructor
function initfamiliar_lynx(u)
  print("a lynx is born ;-)")
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

