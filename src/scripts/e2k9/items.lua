-- used internally to check greatbow and towershield
function item_canuse(u, iname)
  if iname=="towershield" then
    -- only dwarves allowed to use towershield
    return u.race=="dwarf"
  end
  if iname=="greatbow" then
    -- only elves use greatbow
    return u.race=="elf"
  end
  if iname=="plate" then
    -- goblins cannot use plate
    return u.race~="goblin"
  end
  return true
end
