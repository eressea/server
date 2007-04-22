function use_ring_of_levitation(u, amount)
  if u.ship~=nil and amount>0 then
    local mallorn = 0
    for u2 in u.region.units do
      local i = u2:get_item("mallornseed")
      if i>0 then
        u2:use_item("mallornseed", i)
        u2:add_item("seed", i)
        mallorn = mallorn + i
      end
    end
    if mallorn>0 then
      levitate_ship(u.ship, u, mallorn, 2)
    end
  end
  return 0
end
