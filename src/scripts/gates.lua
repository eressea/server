function gate_travel(b, units)
  -- we've found which units we want to exchange, now swap them:
  local u
  for u in units do
    u.region = b.region
    u.building = b
  end
end

function gate_units(b, maxsize)
  local size = maxsize
  local units = {}
  local u
  local first = true

  for u in b.units do
    if first then
      first = false
    else
      if u.number<=size and u.weight<=u.capacity then
        units[u] = u
        size = size - u.number
      end
    end
  end
  return units
end
