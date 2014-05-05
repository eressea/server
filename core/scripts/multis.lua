function kill_nonstarters()
  for f in factions() do
    if f.lastturn==0 then
      kill_faction(f, true)
    end
  end
end

function kill_multis(multis, destructive)
  for idx, fno in ipairs(multis) do  
    local f = get_faction(fno)
      if f~=nil and f.email=="doppelspieler@eressea.de" then
        kill_faction(f, destructive)
      end
  end
end

function mark_multis(multis, block)
  if multis~=nil then
    for idx, fno in ipairs(multis) do
      local f = get_faction(fno)
      if f~=nil and f.email~="doppelspieler@eressea.de" then
        print("* multi-player " .. tostring(f))
        mark_multi(f, block)
      end
    end
  end
end

-- destroy a faction 
-- destructive: kill all of its buildings and the home region, too.

function kill_faction(f, destructive)
  for u in f.units do
    local r = u.region
    local b = u.building
    unit.destroy(u)
    if destructive and b~=nil then
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

local function mark_multi(f, block)
  f.password = "doppelspieler"
  f.email = "doppelspieler@eressea.de"
  f.banner = "Diese Partei steht wegen vermuteten Doppelspiels unter Beobachtung."
  for u in f.units do
    u.race_name = "toad"
    if block and u.building~=nil then
      local found = false
      for u2 in u.region.units do
        if u2.faction.id~=u.faction.id then
          found = true
          break
        end
      end
      if not found then
        u.region.terrain_name = "firewall"
        u.region:set_flag(2) -- RF_BLOCKED
      end
    end
  end
end

local function num_oceans(r)
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

