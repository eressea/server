function creation_message(mage, type, number)
  local msg = message.create("item_create_spell")
  local err = 0
  err = err + msg:set_unit("mage", mage)
  err = err + msg:set_int("number", number)
  err = err + msg:set_resource("item", type)
  if err ~= 0 then
      return nil
  else
      return msg
  end
end

local function create_item(mage, level, name, number)
  local count = number or 1
  mage:add_item(name, count);
  local msg = creation_message(mage, name, count)
  msg:send_faction(mage.faction)
  return level
end

local function create_potion(mage, level, name, force)
  count = math.floor(force * 2 + 0.5)
  return create_item(mage, level, name, count)
end

-- Wasser des Lebens
function create_potion_p2(r, mage, level, force)
  return create_potion(mage, level, "lifepotion", force)
end

-- Siebenmeilentee
function create_potion_p0(r, mage, level, force)
  return create_potion(mage, level, "p0", force)
end

-- Wundsalbe
function create_potion_ointment(r, mage, level, force)
  return create_potion(mage, level, "ointment", force)
end

-- Bauernblut
function create_potion_peasantblood(r, mage, level, force)
  return create_potion(mage, level, "peasantblood", force)
end

-- Pferdeglueck
function create_potion_p9(r, mage, level, force)
  return create_potion(mage, level, "p9", force)
end

-- Schaffenstrunk
function create_potion_p3(r, mage, level, force)
  return create_potion(mage, level, "p3", force)
end

-- Heiltrank
function create_potion_healing(r, mage, level, force)
  return create_potion(mage, level, "healing", force)
end

-- Elixier der Macht
function create_potion_p13(r, mage, level, force)
  return create_potion(mage, level, "p13", force)
end

-- Erschaffe ein Flammenschwert
function create_firesword(r, mage, level, force)
    return create_item(mage, level, "firesword")
end

-- Erschaffe einen Guertel der Trollstaerke
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

-- Erschaffe einen magischen Kraeuterbeutel
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

-- Erschaffe eine Sphaere der Unsichtbarkeit
function create_invisibility_sphere(r, mage, level, force)
  return create_item(mage, level, "sphereofinv")
end

-- Erschaffe einen Guertel der Keuschheit
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

function earn_silver(r, mage, level, force)
  local money = r:get_resource("money")
  local wanted = 50 * force
  local amount = wanted
  if wanted > money then
    amount = money
  end
  r:set_resource("money", money - amount)
  mage:add_item("money", amount)

  local msg = message.create("income")
  msg:set_unit("unit", mage)
  msg:set_region("region", r)
  msg:set_int("mode", 6)
  msg:set_int("wanted", wanted)
  msg:set_int("amount", amount)
  msg:send_faction(mage.faction)
  return level
end
