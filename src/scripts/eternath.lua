if gate_exchange==nil then
  dofile("gates.lua")
end

local function eternath_travellers(b, maxsize)
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

local function eternath_exchange(b1, b2, size)
  local units1 = eternath_travellers(b1, size)
  local units2 = eternath_travellers(b2, size)

  gate_exchange(b1, units1, b2, units2)
end

function eternathgate_action(b)
  if eternathgate == nil then
    eternathgate = b
  else
    eternath_exchange(eternathgate, b, 10)
  end
  return 1
end
