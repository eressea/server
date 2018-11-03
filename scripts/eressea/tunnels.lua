-- Weltentor portal module
local tunnels = {}

local buildings = {}
local targets = {}

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

local function get_target(param)
    local ntargets = table.maxn(targets)
    if ntargets==0 then
        return nil
    end
    local rn = math.fmod(rng_int(), ntargets)
    return targets[rn]
end

local function tunnel_action(b, param)
  local units = tunnel_travelers(b)
  local rto = get_target(param)
  if rto and units then
    eressea.log.debug("Tunnel from " .. tostring(b) .. " [" .. param .. "]")
    for key, u in pairs(units) do
        u.region = rto
        eressea.log.debug("teleported " .. tostring(u) .. " to " .. tostring(rto))
    end
  end
end

function tunnels.init()
    local r, b
    for r in regions() do
        if r:get_key('tnnL') then
            targets[table.maxn(targets)+1] = r
            if (r:get_flag(0)) then
                -- target region is chaotic? nope.
                r:set_flag(0, false)
            end
            if (r.terrain=="ocean") then
                eressea.log.warning("tunnel target at " .. r.x .. "," .. r.y .. " is an ocean, terraforming")
                r = region.create(r.x, r.y, "plain")
            end
        end
        for b in r.buildings do
            if b.type == 'portal' then
                buildings[table.maxn(buildings)+1] = b
            end
        end
    end
end

function tunnels.update()
    for i, b in ipairs(buildings) do
        tunnel_action(b, 'tnnL')
    end
end

return tunnels
