local function tunnel_travellers(b)
  local units = {}
  for u in b.units do
    units[u] = u
  end
  return units
end

targets = nil
ntargets = 0

local function get_target(param)
  if targets == nil then
    targets = {}
    local r
    for r in regions() do
      if r:get_key(param) then
        targets[ntargets] = r
        ntargets = ntargets + 1
      end
    end
  end
  if ntargets==0 then
    return nil
  end
  local rn = math.mod(test.rng_int(), ntargets)
  return targets[rn]
end

-- export, will be called from lc_age()
function tunnel_action(b, param)
  local r = nil
  print("Tunnel from " .. b .. " [" .. param .. "]")
  if tonumber(param)~=nil then
    r = get_region_by_id(tonumber(param))
  end
  if r~=nil then
    local units = tunnel_travelers(b)
    for key, u in pairs(units) do
      local rto = r
      if r==nil then
        rto = get_target(param)
      end
      if rto~=nil then
        u.region = rto
        print(" - teleported " .. u .. " to " .. rto)
      end
    end
  end
  return 1 -- return 0 to destroy
end
