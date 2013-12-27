-- this script contains the action functions for the two portals
-- used on the jadee/wildente wedding island. the two _action functions
-- are used as age() functions for a building_action with b:addaction("name")

if gate_travel==nil then
  loadscript("gates.lua")
end

hellgate = nil
peacegate = nil

local function wedding_travellers(b)
  local units = {}
  
  for u in b.units do
    if u:get_flag("wdgt") then
      units[u] = u
    end
  end
  return units
end

local function wedding_exchange(b1, b2)
  local units1 = wedding_travellers(b1)
  local units2 = wedding_travellers(b2)

  gate_travel(b2, units1)
  gate_travel(b1, units2)
end

function hellgate_action(b)
  if hellgate == nil then
    hellgate = b
  else
    wedding_exchange(hellgate, b)
  end
  return 1
end

function peacegate_action(b)
  if peacegate == nil then
    peacegate = b
  else
    wedding_exchange(peacegate, b)
  end
  return 1
end
