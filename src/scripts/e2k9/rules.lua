-- when appending to this, make sure the item has a canuse-function!
local goblin_denied = " plate lance mallornlance greatbow axe greatsword halberd rustyaxe rustyhalberd towershield "
function item_canuse(u, iname)
  local race = u.race
  if race=="goblin" then
    if string.find(goblin_denied, " " .. iname .. " ") then
      return false
    end
  end
  if iname=="rep_crossbow" then
    -- only dwarves and halflings allowed to use towershield
    return race=="dwarf" or race=="halfling"
  end
  if iname=="towershield" then 
    -- only dwarves allowed to use towershield
    return race=="dwarf"
  end
  if iname=="greatbow" then
    -- only elves use greatbow
    return race=="elf"
  end
  if iname=="plate" then
    -- goblins cannot use plate
    return race~="goblin"
  end
  return true
end
