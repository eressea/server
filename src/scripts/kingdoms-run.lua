function loadscript(name)
  local script = scriptpath .. "/" .. name
  print("- loading " .. script)
  if pcall(dofile, script)==0 then
    print("Could not load " .. script)
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

function update_owners()
-- update the region's owners. currently uses the owner of
-- the largest castle.
  local r
  for r in regions() do
    local lb = nil
    for b in r.buildings do
      if b.type=="castle" and (lb==nil or b.size>lb.size) then
        lb = b
      end
    end
    local u
    if b ~=nil then
      u = b.units()
      if u~=nil and u.faction~=r.owner then
        r.owner = u.faction
      end
    end
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

  -- DISABLED: plan_monsters()
  process_orders()
  update_owners()
  
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
  -- loadscript("spells.lua")
  loadscript("extensions.lua")
  loadscript("kingdoms/extensions.lua")
  process(orderfile)
end

