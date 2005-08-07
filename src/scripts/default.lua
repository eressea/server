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

  -- run the turn:
  read_orders(orders)  

  plan_monsters()
  process_orders()
  
  -- initialize starting equipment for new players
  -- probably not necessary, since mapper sets new players, not server
  add_equipment("conquesttoken", 1);
  add_equipment("wood", 30);
  add_equipment("stone", 30);
  add_equipment("money", 4200);

  -- use newfactions file to place out new players
  autoseed(basepath .. "/newfactions", true)

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

