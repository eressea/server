function test_handler()

  local function msg_handler(u, evt)
    str = evt:get_string(0)
    u2 = evt:get_unit(1)
    print(u)
    print(u2)
    print(str)
  end

  plain = terraform(0, 0, "plain")
  skill = 8

  f = add_faction("enno@eressea.de", "orc", "de")
  f.age = 20

  u = add_unit(f, plain)
  u.number = 1
  u:add_item("money", u.number*100)
  u:clear_orders()
  u:add_order("NUMMER PARTEI test")
  u:add_handler("message", msg_handler)
  msg = "BOTSCHAFT EINHEIT " .. itoa36(u.id) .. " Du~Elf~stinken"

  f = add_faction("enno@eressea.de", "elf", "de")
  f.age = 20

  u = add_unit(f, plain)
  u.number = 1
  u:add_item("money", u.number*100)
  u:clear_orders()
  u:add_order("NUMMER PARTEI eviL")
  u:add_order(msg)

  process_orders()
  write_reports() 
end

function test_combat()

  plain = terraform(0, 0, "plain")
  skill = 8

  f = add_faction("enno@eressea.de", "orc", "de")
  f.age = 20

  u = add_unit(f, plain)
  u.number = 100
  u:add_item("money", u.number*100)
  u:add_item("sword", u.number)
  u:set_skill("sk_melee", skill)
  u:clear_orders()
  u:add_order("NUMMER PARTEI test")
  u:add_order("KÄMPFE")
  u:add_order("BEFÖRDERUNG")
  attack = "ATTACKIERE " .. itoa36(u.id)

  f = add_faction("enno@eressea.de", "elf", "de")
  f.age = 20

  u = add_unit(f, plain)
  u.number = 100
  u:add_item("money", u.number*100)
  u:add_item("sword", u.number)
  u:set_skill("sk_melee", skill+2)
  u:clear_orders()
  u:add_order("NUMMER PARTEI eviL")
  u:add_order("KAEMPFE")
  u:add_order(attack)

  process_orders()
  write_reports() 
end

function test_rewards()
  -- this script tests manufacturing and fighting.

  plain = terraform(0, 0, "plain")
  skill = 5

  f = add_faction("enno@eressea.de", "human", "de")
  f.age = 20
  u = add_unit(f, plain)
  u.number = 10
  u:add_item("money", u.number*100)
  u:add_item("greatbow", u.number)
  u:set_skill("sk_bow", skill)
  u:clear_orders()
  u:add_order("KAEMPFE")
  attack = "ATTACKIERE " .. itoa36(u.id)

  u = add_unit(f, plain)
  u.number = 7
  u:add_item("money", u.number*100)
  u:add_item("mallorn", u.number*10)
  u:set_skill("sk_weaponsmithing", 7)
  u:clear_orders()
  u:add_order("KAEMPFE NICHT")
  u:add_order("MACHEN Elfenbogen")
  u:add_order("NUMMER PARTEI test")

  f = add_faction("enno@eressea.de", "elf", "de")
  f.age = 20
  u = add_unit(f, plain)
  u.number = 7
  u:add_item("money", u.number*100)
  u:add_item("greatbow", u.number)
  u:set_skill("sk_bow", skill)
  u:clear_orders()
  u:add_order("KAEMPFE HINTEN")
  u:add_order(attack)

  u = add_unit(f, plain)
  u.number = 7
  u:add_item("money", u.number*100)
  u:add_item("mallorn", u.number*10)
  u:set_skill("sk_weaponsmithing", 7)
  u:clear_orders()
  u:add_order("KAEMPFE NICHT")
  u:add_order("MACHEN Elfenbogen")
  u:add_order("NUMMER PARTEI eviL")

  u = add_unit(f, plain)
  u.number = 7
  u:add_item("money", u.number*100)
  u:add_item("mallorn", u.number*10)
  u:set_skill("sk_weaponsmithing", 7)
  u:clear_orders()
  u:add_order("KAEMPFE NICHT")

  items = { "hornofdancing", "trappedairelemental", 
    "aurapotion50", "bagpipeoffear",
    "instantartacademy", "instantartsculpture" }
  for index in items do  
    u:add_item(items[index], 1)
    u:add_order('@BENUTZEN "' .. get_string("de", items[index]) .. '"')
  end
  u:add_order("NUMMER PARTEI eviL")

  process_orders()
  write_reports() 
end

function test_give()
  plain = terraform(0, 0, "plain")
  f = add_faction("enno@eressea.de", "human", "de")
  f.age = 20
  u = add_unit(f, plain)
  u.number = 10
  u:add_item("money", u.number*100)
  u:clear_orders()
  u:add_order("MACHE TEMP eins")
  u:add_order("REKRUTIERE 1")
  u:add_order("ENDE")
  u:add_order("GIB TEMP eins ALLES silber")
  u:add_order("NUMMER PARTEI test")
  
  process_orders()
  write_reports() 
end

function test_write()
  read_game("24")
  read_orders("befehle")
  process_orders()
  write_reports() 
  write_game("25")
end

function move_north(u)
  for order in u.orders do
    print(order)
  end
  u:clear_orders()
  u:add_order("NACH NORDEN")
end

function test_monsters()
  read_game("23")

  -- magrathea = get_region(-67, -5)
  local magrathea = get_region(0, 0)
  if magrathea ~= nil then
    if pcall(dofile, scriptpath .. "/ponnuki.lua") then
      init_ponnuki(magrathea)
    else
      print("could not open ponnuki")
    end
  end

  set_brain("braineater", move_north)
  plan_monsters()
  process_orders()
end

function test_parser()
  -- this script tests the changes to quotes

  plain = terraform(0, 0, "plain")
  skill = 5

  f = add_faction("enno@eressea.de", "human", "de")
  f.age = 20
  u = add_unit(f, plain)
  u.number = 10
  u:clear_orders()
  u:add_order("Nummer Partei test")
  u:add_order("BENENNE PARTEI \"Diese Partei heisst \\\"Enno's Schergen\\\".\"")
  u:add_order("BENENNE EINHEIT \"Mein Name ist \\\"Enno\\\".\"")

  process_orders()
  write_reports() 
  write_game("parser")
end

function test_fail()
  plain = terraform(0, 0, "plain")
  skill = 5

  f = add_faction("enno@eressea.de", "human", "de")
  print(f)
end

test_fail()
-- test_handler()
-- test_parser()
-- test_monsters()
-- test_combat()
-- test_rewards()
-- test_give()
-- test_write()
-- read_game("test")
-- write_game("test")
