-- this script contains the action functions for the two portals
-- used on the jadee/wildente wedding island. the two _action functions
-- are used as age() functions for a building_action with b:addaction("name")

hellgate = nil
peacegate = nil

function gate_exchange(b1, b2)
  local units1 = {}
  local units2 = {}
  local u
  for u in b1.units do
    if u:get_flag("wdgt") then
      units1[u.no] = u
    end
  end
  for u in b2.units do
    if u:get_flag("wdgt") then
      units2[u.no] = u
    end
  end
  for id in units1 do
    u = units1[id]
    u.region = b2.region
    u.building = b2
  end
  for id in units2 do
    u = units2[id]
    u.region = b1.region
    u.building = b1
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
