if gate_travel==nil then
  dofile(scriptpath .. "/gates.lua")
end

buildings = {}

function portal_exchange(b1, param)
  b2 = buildings[param]
  if buildings[param] == nil then
    buildings[param] = b1
  else
    u1 = gate_units(b1, 100)
    u2 = gate_units(b2, 100)
    gate_travel(b1, u2)
    gate_travel(b2, u1)
  end
  return 1
end
