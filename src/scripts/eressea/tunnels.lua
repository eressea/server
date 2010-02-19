local function tunnel_travelers(b)
  local units = nil
  for u in b.units do
    if units==nil then
      units = {}
    end
    units[u] = u
  end
  return units
end

targets = nil
ntargets = 0

local function get_target(param)
  -- print("finding targets: " .. param)
  if targets == nil then
    targets = {}
    local r
    for r in regions() do
      if r:get_key(param) then
        if (r:get_flag(0)) then
          r:set_flag(0, false)
        end
        if (r.terrain=="ocean") then
          r = region.create(r.x, r.y, "plain")
        end
        targets[ntargets] = r
        ntargets = ntargets + 1
        -- print("target: " .. tostring(r))
      end
    end
  end
  if ntargets==0 then
    return nil
  end
  local rn = math.mod(rng_int(), ntargets)
  return targets[rn]
end

-- export, will be called from lc_age()
function tunnel_action(b, param)
  local r = nil
  if tonumber(param)~=nil then
    r = get_region_by_id(tonumber(param))
  end
  local units = tunnel_travelers(b)
  if units~=nil then
    print("Tunnel from " .. tostring(b) .. " [" .. param .. "]")
    for key, u in pairs(units) do
      local rto = r
      if r==nil then
        rto = get_target(param)
      end
      if rto~=nil then
        u.region = rto
        print(" - teleported " .. tostring(u) .. " to " .. tostring(rto))
      end
    end
  end
  return 1 -- return 0 to destroy
end
