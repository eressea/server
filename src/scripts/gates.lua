function gate_exchange(b1, units1, b2, units2)
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
