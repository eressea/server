-- used internally to check greatbow and towershield
function item_canuse(u, iname)
  if iname=="towershield" then
    return u.race=="dwarf"
  end
  if iname=="greatbow" then
    return u.race=="elf"
  end
  return true
end
