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

  -- initialize other scripts
  local magrathea = get_region(-67, -5)
  if magrathea~=nil and init_ponnuki~=nil then
    init_ponnuki(magrathea)
    return
  end

  -- run the turn:
  read_orders(orders)
  plan_monsters()
  process_orders()
  
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

scripts = { "ponnuki.lua", "wedding-jadee.lua" }

-- orderfile: contains the name of the orders.
if orderfile==nil then
  print "you must specify an orderfile"
else
  for index in scripts do
    local script = scriptpath .. scripts[index]
    if pcall(dofile, script)==0 then
      print("Could not load " .. script)
    end
  end
  process(orderfile)
end

