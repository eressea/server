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

function building_taxes(b)
  btype = b.type
  bsize = b.size
  if btype=="castle" then
    if bsize>=6250 then return 0.05 end
    if bsize>=1250 then return 0.04 end
    if bsize>=250 then return 0.03 end
    if bsize>=50 then return 0.02 end
    if bsize>=10 then return 0.01 end
  elseif btype=="watch" then
    if bsize>=10 then return 0.01 end
    if bsize>=5 then return 0.005 end
  end
  return 0.0
end
