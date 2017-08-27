-- DEPRECATED
if not config.wedding or config.wedding==0 then return nil end

-- this script contains the action functions for the two portals
-- used on the jadee/wildente wedding island. the two _action functions
-- are used as age() functions for a building_action with b:addaction("name")
-- this module is deprecated, because it puts functions in the global environment for at_building_action

local gates = require('eressea.gates')

local hellgate = nil
local peacegate = nil

local function wedding_travellers(b)
  local units = {}
  
  for u in b.units do
    if u:get_flag('wdgt') then
      units[u] = u
    end
  end
  return units
end

local function wedding_exchange(b1, b2)
  local units1 = wedding_travellers(b1)
  local units2 = wedding_travellers(b2)

  gates.travel(b2, units1)
  gates.travel(b1, units2)
end

function hellgate_action(b)
    hellgate = b
    return 1
end

function peacegate_action(b)
    peacegate = b
    return 1
end

local wedding = {}

function wedding.update()
    if peacegate and hellgate then
        wedding_exchange(peacegate, hellgate)
    else
        eressea.log.warning("hellgate or peacegate not found!")
    end
end

return wedding
