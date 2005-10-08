function creation_message(mage, type)
  msg = message("item_create_spell")
  msg:set_unit("mage", mage)
  msg:set_int("number", 1)
  msg:set_resource("item", type)
  return msg
end

-- Erschaffe ein Flammenschwert
function create_firesword(r, mage, level, force)
  mage:add_item("firesword", 1);

  local msg = creation_message(mage, "firesword")
  msg:send_faction(mage.faction)
  return level
end

-- Erschaffe einen Gürtel der Trollstärke
function create_trollbelt(r, mage, level, force)
  mage:add_item("trollbelt", 1);

  local msg = creation_message(mage, "trollbelt")
  msg:send_faction(mage.faction)
  return level
end

-- Erschaffe einen Ring der Unsichtbarkeit
function create_roi(r, mage, level, force)
  mage:add_item("roi", 1);

  local msg = creation_message(mage, "roi")
  msg:send_faction(mage.faction)
  return level
end

-- Erschaffe einen Ring der flinken Finger
function create_roqf(r, mage, level, force)
  mage:add_item("roqf", 1);

  local msg = creation_message(mage, "roqf")
  msg:send_faction(mage.faction)
  return level
end

-- Erschaffe ein Amulett des wahren Sehens
function create_roi(r, mage, level, force)
  mage:add_item("aots", 1);

  local msg = creation_message(mage, "aots")
  msg:send_faction(mage.faction)
  return level
end

-- Erschaffe einen magischen Kräuterbeutel
function create_magicherbbag(r, mage, level, force)
  mage:add_item("aots", 1);

  local msg = creation_message(mage, "magicherbbag")
  msg:send_faction(mage.faction)
  return level
end

-- Erschaffe einen Taktikkristal
function create_dreameye(r, mage, level, force)
  mage:add_item("", 1);

  local msg = creation_message(mage, "dreameye")
  msg:send_faction(mage.faction)
  return level
end

