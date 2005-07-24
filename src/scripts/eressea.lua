function loadscript(name)
  local script = scriptpath .. "/" .. name
  print("- loading " .. script)
  if pcall(dofile, script)==0 then
    print("Could not load " .. script)
  end
end

function change_locales()
  local localechange = { }
  -- local localechange = { en = { "y7hq" } }
  
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
    "eternath.lua",
    "wedding-jadee.lua", 
    "ponnuki.lua"
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
  file = "" .. get_turn()
  if read_game(file)~=0 then
    print("could not read game")
    return -1
  end
  init_summary()

  -- kill multi-players (external script)
  loadscript("eressea-multis.lua")
  kill_multis()

  -- run the turn:
  if read_orders(orders) ~= 0 then
    print("could not read " .. orders)
    return -1
  end
  run_scripts()

  -- initialize starting equipment for new players
  add_equipment("conquesttoken", 1);
  add_equipment("log", 30);
  add_equipment("stone", 30);
  add_equipment("money", 4200);

  plan_monsters()
  process_orders()

  change_locales()
  
  -- use newfactions file to place out new players
  autoseed(basepath .. "/newfactions")

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


--
-- main body of script
--

-- orderfile: contains the name of the orders.
if orderfile==nil then
  print "you must specify an orderfile"
else
  process(orderfile)
end

