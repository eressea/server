-- when appending to this, make sure the item has a canuse-function!
local goblin_denied = " plate lance mallornlance greatbow axe greatsword halberd rustyaxe rustyhalberd towershield scale "
function item_canuse(u, iname)
  local race = u.race
  if race=="goblin" then
    if string.find(goblin_denied, " " .. iname .. " ") then
      return false
    end
  end
  if iname=="rep_crossbow" then
    -- only dwarves and halflings allowed to use repeating crossbow
    return race=="dwarf" or race=="halfling"
  end
  if iname=="scale" then
    -- only dwarves and halflings can use scale
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
  return true
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

-- the "raindance" spell
function raindance(r, mage, level, force)
  if (create_curse(mage, r, "blessedharvest", force, 1+force*2, 100 * force)) then
    -- slightly crooked way of reporting an action to everyone in the region
    local msg = message.create("raindance_effect")
    msg:set_unit("mage", mage)
    if (msg:report_action(r, mage, 3)) then
      local msg2 = message.create("raindance_effect")
      msg2:set_unit("mage", nil)
      msg2:report_action(r, mage, 4)
    end
  end
  return level
end

-- the "blessed harvest" spell
function blessedharvest(r, mage, level, force)
  if create_curse(mage, r, "blessedharvest", force, 1+force*2, 50 * force) then
    -- slightly crooked way of reporting an action to everyone in the region
    local msg = message.create("harvest_effect")
    msg:set_unit("mage", mage)
    if (msg:report_action(r, mage, 3)) then
      local msg2 = message.create("harvest_effect")
      msg2:set_unit("mage", nil)
      msg2:report_action(r, mage, 4)
    end
    for idx, rn in ipairs(r.adj) do
      -- nur landregionen haben moral>=0
      if r.morale>=0 then
        create_curse(mage, r, "blessedharvest", force, force*2, 50 * force)
      end
    end
  end
  return level
end
