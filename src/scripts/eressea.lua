function run_scripts()
  scripts = { 
    "wedding-jadee.lua", 
    "eternath.lua",
    "ponnuki.lua"
  }
  for index in scripts do
    local script = scriptpath .. "/" .. scripts[index]
    print("- loading " .. script)
    if pcall(dofile, script)==0 then
      print("Could not load " .. script)
    end
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

  -- initialize starting equipment for new players
  -- probably not necessary, since mapper sets new players, not server
  add_equipment("conquesttoken", 1);
  add_equipment("wood", 30);
  add_equipment("stone", 30);
  add_equipment("money", 2000 + get_turn() * 10);

  -- run the turn:
  read_orders(orders)
  run_scripts()

  plan_monsters()
  process_orders()

  -- localechange = { de = { "bLub" } }
  localechange = {  }
  for loc, flist in localechange do
	  for index, name in flist do
      f = get_faction(atoi36(name))
      if f ~= nil then
        f.locale = loc
  			print("LOCALECHANGE ", f, loc)
			end
    end
  end

  write_passwords()
  write_reports()

  write_emails()

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

