function test_sail()
  r0 = terraform(0, 0, "plain")

  orcs = add_faction("enno@eressea.de", "orc", "de")
  orcs.age = 20

  orc = add_unit(orcs, r0)
  orc.number = 1
  orc:add_item("speedsail", orc.number)

  orc:clear_orders()
  orc:add_order("NUMMER PARTEI orcs")
  orc:add_order("NUMMER EINHEIT orc")
  orc:add_order("BENENNE EINHEIT Orks")
  orc:add_order("ZEIGEN \"Sonnensegel\"")

  process_orders()
  write_reports() 
end

function test_movement()
  west = direction("west")
  east = direction("east")

  r0 = terraform(0, 0, "plain")
  r1 = terraform(1, 0, "desert")
  r2 = terraform(2, 0, "glacier")
  r3 = terraform(3, 0, "plain")
  r4 = terraform(4, 0, "glacier")

  r0:set_road(east, 1.0)
  r1:set_road(west, 1.0)
  r1:set_road(east, 1.0)
  r2:set_road(west, 1.0)
  r2:set_road(east, 1.0)
  r3:set_road(west, 1.0)
  r3:set_road(east, 1.0)
  r4:set_road(west, 1.0)

  orcs = add_faction("enno@eressea.de", "orc", "de")
  orcs.age = 20

  orc = add_unit(orcs, r0)
  orc.number = 10
  orc:add_item("money", orc.number*10)
  orc:add_item("horse", orc.number*3)
  orc:set_skill("sk_riding", 10)

  bugs = add_faction("enno@eressea.de", "insect", "de")
  bugs.age = 20

  bug = add_unit(bugs, r0)
  bug.number = 1
  bug:add_item("money", bug.number*10)

  orc:clear_orders()
  orc:add_order("NUMMER PARTEI orcs")
  orc:add_order("NUMMER EINHEIT orc")
  orc:add_order("BENENNE EINHEIT Orks")
  orc:add_order("ROUTE O O O P P O W W W W")
  orc:add_order("GIB 0 ALLES Steine")
  orc:add_order("GIB 0 ALLES Holz")
  orc:add_order("TRANSPORTIEREN " .. itoa36(bug.id))

  bug:clear_orders()
  bug:add_order("NUMMER PARTEI bugs")
  bug:add_order("NUMMER EINHEIT bug")
  bug:add_order("BENENNE EINHEIT Käfer")
  bug:add_order("GIB 0 ALLES Steine")
  bug:add_order("GIB 0 ALLES Holz")
  bug:add_order("FAHREN " .. itoa36(orc.id))

  u = add_unit(orcs, r0)
  u.number = 1
  u:add_item("horse", u.number*3)
  u:add_item("money", u.number*10)
  u:set_skill("sk_riding", 10)
  u:set_skill("sk_stealth", 2)
  u:clear_orders()
  u:add_order("FOLGEN EINHEIT " .. itoa36(bug.id))
  u:add_order("NACH W")
  u:add_order("NUMMER EINHEIT foLg")
  u:add_order("BENENNE EINHEIT Verfolger")

  u2 = add_unit(orcs, r0)
  u2.number = 1
  u2:add_item("horse", u2.number*3)
  u2:add_item("money", u.number*10)
  u2:set_skill("sk_riding", 10)
  u2:set_skill("sk_stealth", 2)
  u2:clear_orders()
  u2:add_order("FOLGEN EINHEIT nix")
  u2:add_order("NUMMER EINHEIT Last")
  u2:add_order("BENENNE EINHEIT Verfolger-Verfolger")


  process_orders()
  write_reports() 
end


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

test_sail()
-- test_movement()
-- test_fail()
-- test_handler()
-- test_parser()
-- test_monsters()
-- test_combat()
-- test_rewards()
-- test_give()
-- test_write()
-- read_game("test")
-- write_game("test")
