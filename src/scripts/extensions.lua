function initfamiliar_lynx(u)
  print("a lynx is born :-)")
end

function peasant_getresource(u)
  return u.region:get_resource("peasant")
end

function peasant_changeresource(u, delta)
  local p = u.region:get_resource("peasant")
  p = p + delta
  if p < 0 then
    p = 0
  end
  u.region:set_resource("peasant", p)
  return p
end

function hp_getresource(u)
  return u.hp
end

function hp_changeresource(u, delta)
  local hp = u.hp + delta
  
  if hp < u.number then
    if hp < 0 then
      hp = 0
    end
    u.number = hp
  end
  u.hp = hp
  return hp
end
