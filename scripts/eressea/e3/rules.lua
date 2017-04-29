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
