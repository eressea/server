function creation_message(mage, type)
  msg = message("item_create_spell")
  msg:set_unit("mage", mage)
  msg:set_int("number", 1)
  msg:set_resource("item", type)
  return msg
end

-- Erschaffe ein Flammenschwert
local function create_item(mage, level, name)
  local msg = creation_message(mage, name)
  msg:send_faction(mage.faction)
  mage:add_item(name, 1);
  return level
end

-- Erschaffe ein Flammenschwert
function create_firesword(r, mage, level, force)
  return create_item(mage, level, "firesword")
end

-- Erschaffe einen Gürtel der Trollstärke
function create_trollbelt(r, mage, level, force)
  return create_item(mage, level, "trollbelt")
end

-- Erschaffe einen Ring der Unsichtbarkeit
function create_roi(r, mage, level, force)
  return create_item(mage, level, "roi")
end

-- Erschaffe einen Ring der flinken Finger
function create_roqf(r, mage, level, force)
  return create_item(mage, level, "roqf")
end

-- Erschaffe ein Amulett des wahren Sehens
function create_aots(r, mage, level, force)
  return create_item(mage, level, "aots")
end

-- Erschaffe einen magischen Kräuterbeutel
function create_magicherbbag(r, mage, level, force)
  return create_item(mage, level, "magicherbbag")
end

-- Erschaffe einen Taktikkristal
function create_dreameye(r, mage, level, force)
  return create_item(mage, level, "dreameye")
end

-- Erschaffe einen Antimagiekristall
function create_antimagic(r, mage, level, force)
  return create_item(mage, level, "antimagic")
end

-- Erschaffe eine Sphäre der Unsichtbarkeit
function create_invisibility_sphere(r, mage, level, force)
  return create_item(mage, level, "sphereofinv")
end

-- Erschaffe einen Gürtel der Keuschheit
function create_chastitybelt(r, mage, level, force)
  return create_item(mage, level, "ao_chastity")
end

-- Erschaffe ein Runenschwert
function create_runesword(r, mage, level, force)
  return create_item(mage, level, "runesword")
end

-- Erschaffe ein Aurafokus
function create_focus(r, mage, level, force)
  return create_item(mage, level, "aurafocus")
end

-- Erschaffe einen Ring der Macht
function create_rop(r, mage, level, force)
  return create_item(mage, level, "rop")
end

-- Erschaffe einen Ring der Regeneration
function create_ror(r, mage, level, force)
  return create_item(mage, level, "ror")
end

-- Erschaffe einen Zauberbeutel
function create_bagofholding(r, mage, level, force)
  return create_item(mage, level, "magicbag")
end

-- TODO:
function earnsilver(r, mage, level, force)
  local money = r:get_resource("money")
  local wanted = 50 * force
  local amount = wanted
  if wanted > money then
    amount = money
  end
  r:set_resource("money", money - amount)
  mage:add_item("money", amount)

  msg = message("income")
  msg:set_unit("unit", mage)
  msg:set_region("region", r)
  msg:set_int("mode", 6)
  msg:set_int("wanted", wanted)
  msg:set_int("amount", amount)
  msg:send_faction(mage.faction)
end
