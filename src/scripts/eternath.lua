function eternath_travellers(b, maxsize)
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

function eternath_exchange(b1, b2)
  -- identify everyone who is travelling, first:
  local units1 = eternath_travellers(b1, 10)
  local units2 = eternath_travellers(b2, 10)

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

function eternathgate_action(b)
  if eternathgate == nil then
    eternathgate = b
  else
    eternath_exchange(eternathgate, b)
  end
  return 1
end
