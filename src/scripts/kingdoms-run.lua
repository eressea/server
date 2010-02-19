-- the locales that this gameworld supports.
local locales = { "de", "en" }

function loadscript(name)
  local script = scriptpath .. "/" .. name
  print("- loading " .. script)
  if pcall(dofile, script)==0 then
    print("Could not load " .. script)
  end
end

function update_resources()
-- remaining contents of region pool rots
-- wood falls from trees
  local r
  for r in regions() do
    local item
    for item in r.items do
      local num = math.ceil(r:get_item(item)/2)
      r:add_item(item, -num)
    end

    local wood = math.floor(r:get_resource("tree")/20)
    if wood>0 then
      r.add_item("log", wood)
    end
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

  process_orders()
  update_owners()
  update_resources()
  
  -- use newfactions file to place out new players
  autoseed(basepath .. "/newfactions", true)

  write_files(locales)

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
  loadscript("default.lua")
  loadscript("extensions.lua")
  loadscript("kingdoms/extensions.lua")
  process(orderfile)
end

