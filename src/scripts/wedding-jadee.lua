-- this script contains the action functions for the two portals
-- used on the jadee/wildente wedding island. the two _action functions
-- are used as age() functions for a building_action with b:addaction("name")

hellgate = nil
peacegate = nil

function gate_exchange(b1, b2)
  local units = {}
  local u
  for u in b1.units do
    units[u.no] = u
  end
  for u in b2.units do
    u.region = b1.region
    u.building = b1
  end
  for id in units do
    u = units[id]
    u.region = b2.region
    u.building = b2
  end
end

function hellgate_action(b)
  if hellgate == nil then
    hellgate = b
  else
    gate_exchange(hellgate, b)
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
