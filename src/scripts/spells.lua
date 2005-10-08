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

