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
    local ntargets = #targets
    if ntargets == 0 then
        eressea.log.error("No tunnel targets for  [" .. param .. "]")
        return nil
    end
    local rn = math.fmod(rng_int(), ntargets)
    local t = targets[rn + 1]
    if not t then
        eressea.log.error("NULL target for  [" .. param .. "]" .. " at index " .. rn)
    end
    return t
end

local function tunnel_action(b, param)
  local units = tunnel_travelers(b)
  eressea.log.info("Tunnel from " .. tostring(b) .. " [" .. param .. "]")
  if units then
    for _, u in pairs(units) do
        local rto = get_target(param)
        if not rto then
            break
        end
        u.region = rto
        eressea.log.info("teleported " .. tostring(u) .. " to " .. tostring(rto))
    end
  elseif not units then 
    eressea.log.info("No units in tunnel " .. tostring(b) .. " [" .. param .. "]")
  elseif not rto then 
    eressea.log.error("No target for tunnel " .. tostring(b) .. " [" .. param .. "]")
  end
  -- debug code to find bugs:
  for u in b.region.units do
    if u.building == b then
        eressea.log.error("Did not teleport " .. tostring(u) .. " from tunnel " .. tostring(b))
    end
  end
end

function tunnels.init()
    local r, b
    for r in regions() do
        if r:get_key('tnnL') then
            table.insert(targets, r)
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
                table.insert(buildings, b)
            end
        end
    end
    eressea.log.info("Found " .. #targets .. " tunnel targets")
end

function tunnels.update()
    for i, b in ipairs(buildings) do
        tunnel_action(b, 'tnnL')
    end
end

return tunnels
