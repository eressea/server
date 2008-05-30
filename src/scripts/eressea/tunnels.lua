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
  if tonumber(param)~=nil then
    r = get_region_by_id(tonumber(param))
  end
  if r~=nil then
    local units = tunnel_travelers(b)
    for key, u in pairs(units) do
      if r==nil then
        u.region = get_target(param)
      else
        u.region = r
      end
    end
  end
end
