-- when appending to this, make sure the item has a canuse-function!
local goblin_denied = " plate lance mallornlance greatbow axe greatsword halberd rustyaxe rustyhalberd towershield "
function item_canuse(u, iname)
  if u.race=="goblin" then
    if string.find(goblin_denied, " " .. iname .. " ") then
      return false
    end
  end
  if iname=="towershield" or iname=="rep_crossbow" then
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
