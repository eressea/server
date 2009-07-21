-- the locales that this gameworld supports.
local locales = { "de", "en" }
local confirmed_multis = { 
}
local suspected_multis = { 
}

function num_oceans(r)
  local oceans = 0
  local p = r:next(5)
  for d = 0,5 do
    local n = r:next(d)
    if p.terrain~="ocean" and n.terrain=="ocean" then
      oceans = oceans +1
    end
    p = n
  end
  return oceans
end

-- destroy a faction and all of its buildings.
-- destroy the home region, too
function kill_faction(f)
  for u in f.units do
    local r = u.region
    local b = u.building
    unit.destroy(u)
    if b~=nil then
      building.destroy(b)
      local nuke = true
      for v in r.units do
        if v.faction.id~=f.id then
          -- print("cannot nuke: " .. tostring(v.faction))
          nuke = false
          break
        end
      end
      r.terrain_name = nil
      if nuke and num_oceans(r)<=1 then
        -- print("nuke!")
        r.terrain = "ocean"
      else
        -- print("cannot nuke: > 1 oceans")
        r.terrain = "glacier"
        r.peasants = 10
        r:set_resource("money", 100)
        b = building.create(r, "monument")
        b.size = 1
        b.name = "Memento Mori"
        b.info = "Eine kleine " .. translate("race::" .. f.race .."_x") .. "-Statue erinnert hier an ein verschwundenes Volk"
      end
    end
  end
  faction.destroy(f)
end

function kill_nonstarters()
  for f in factions() do
    if f.lastturn==1 then
      kill_faction(f)
    end
  end
end

function kill_multis(multis)
  for idx, fno in ipairs(multis) do  
    local f = get_faction(fno)
      if f~=nil and f.email=="doppelspieler@eressea.de" then
        kill_faction(f)
      end
  end
end

function mark_multis(multis)
  if multi~=nil and multis~=nil then
    for idx, fno in ipairs(multis) do
      local f = get_faction(fno)
      if f~=nil and f.email~="doppelspieler@eressea.de" then
        mark_multi(f)
      end
    end
  end
end

function loadscript(name)
  local script = scriptpath .. "/" .. name
  print("- loading " .. script)
  if pcall(dofile, script)==0 then
    print("Could not load " .. script)
  end
end

function change_locales()
  -- local localechange = { }
  local localechange = { de = { "rtph" } }
  
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
    "e3a/multi.lua",
  }
  for index, value in pairs(scripts) do
    loadscript(value)
  end
end

function best_scores(n)
  local f, numf, top

  numf = 0
  top = { }
  for f in factions() do
    numf = numf + 1
    local r = 0
    local score = f.score
    for i = 1,n do
      if top[i]==nil then
        top[i] = f
        break
      end
      if top[i].score<score then
        for j = n,i+1,-1 do
          top[j]=top[j-1]
        end
        top[i] = f
        break
      end
    end
  end
  return top
end

function write_statistics()
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
  if read_orders(orders) ~= 0 then
    print("could not read " .. orders)
    return -1
  end

  kill_multis(confirmed_multis)
  -- plan_monsters()

  local nmrs = get_nmrs(1)
  --  nmrs = 0
  if maxnmrs == nil then
      maxnmrs = 30
  end
  if nmrs >= maxnmrs then
    print("Shit. More than " .. maxnmrs .. " factions with 1 NMR (" .. nmrs .. ")")
    write_summary()
    write_game("aborted.dat")
    return -1
  end
  print (nmrs .. " Factions with 1 NMR")

  process_orders()

  -- create new monsters:
  spawn_dragons()
  spawn_undead()
  -- spawn_braineaters(0.25)
  -- spawn_ents()

  kill_nonstarters()
  mark_multis(suspected_multis)
  -- post-turn updates:
  update_guards()
  update_scores()

  change_locales()

  -- use newfactions file to place out new players
  -- autoseed(basepath .. "/newfactions", false)

  write_files(locales)
  write_statistics()

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
