print("loaded rules.lua")
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

function building_protection(b, u)
  return 1
end

function building_taxes(b, blevel)
  btype = b.type
  if btype=="castle" then
    return blevel * 0.01
  elseif btype=="watch" then
    return blevel * 0.005
  end
  return 0.0
end
