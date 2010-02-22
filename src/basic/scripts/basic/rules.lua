function item_canuse(u, iname)
  local race = u.race
  if iname=="greatbow" then
    -- only elves use greatbow
    return race=="elf"
  end
  return true
end
