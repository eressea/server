function use_ring_of_levitation(u, amount)
  if u.ship~=nil and amount>0 then
    local mallorn = 0
    for u2 in u.region.units do
      if u2.ship==u.ship then
        local i = u2:get_item("mallornseed")
        if i>0 then
          u2:use_pooled("mallornseed", i)
          u2:use_pooled("seed", i)
          mallorn = mallorn + i
        end
      end
    end
    if mallorn>0 then
      levitate_ship(u.ship, u, mallorn, 2)
    end
  end
  return 0
end
