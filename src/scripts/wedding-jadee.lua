-- this script contains the action functions for the two portals
-- used on the jadee/wildente wedding island. the two _action functions
-- are used as age() functions for a building_action with b:addaction("name")

hellgate = nil
peacegate = nil

function wedding_travellers(b)
  local units = {}
  
  for u in b.units do
    if u:get_flag("wdgt") then
      units[u] = u
    end
  end
  return units
end

function wedding_exchange(b1, b2)
  local units1 = wedding_travellers(b1)
  local units2 = wedding_travellers(b2)

  -- we've found which units we want to exchange, now swap them:
  local u
  for u in units1 do
    u.region = b2.region
    u.building = b2
  end
  for u in units2 do
    u.region = b1.region
    u.building = b1
  end
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
    gate_exchange(peacegate, b)
  end
  return 1
end
