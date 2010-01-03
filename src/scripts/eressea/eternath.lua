require "gates"

local function eternath_exchange(b1, b2, size)
  local units1 = gate_units(b1, size)
  local units2 = gate_units(b2, size)

  gate_travel(b2, units1)
  gate_travel(b1, units2)
end

function eternathgate_action(b)
  if eternathgate == nil then
    eternathgate = b
  else
    eternath_exchange(eternathgate, b, 10)
  end
  return 1
end
