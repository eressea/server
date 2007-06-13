function loadscript(name)
  local script = scriptpath .. "/" .. name
  print("- loading " .. script)
  if pcall(dofile, script)==0 then
    print("Could not load " .. script)
  end
end

function run_scripts()
  scripts = { 
    "spells.lua",
    "extensions.lua",
    "familiars.lua",
    "write_emails.lua"
  }
  for index, name in pairs(scripts) do
    loadscript(name)
  end
end

function equip_new_faction(u)
  b = add_building(u.region, "castle")
  b.size = 10
  u.building = b
end

function process(orders)
  file = "" .. get_turn()
  if read_game(file)~=0 then
    print("could not read game")
    return -1
  end
  init_summary()

  -- run the turn:
  if read_orders(orders) ~= 0 then
    print("could not read " .. orders)
    return -1
  end
  run_scripts()

  plan_monsters()

  local nmrs = get_nmrs(1)
  if nmrs >= 70 then
    print("Shit. More than 70 factions with 1 NMR (" .. nmrs .. ")")
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

