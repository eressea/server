function mkunit(f, r, num)
  u = add_unit(f, r)
  u.number = num
  u:add_item("money", num*10)
  u:clear_orders()
  return u
end

function test_movement()
  west = direction("west")
  east = direction("east")

  -- im westen ohne strassen
  ocean = terraform(-3, 0, "ocean")
  w2 = terraform(-2, 0, "plain")
  w1 = terraform(-1, 0, "plain")

  -- im osten mit strassen
  r0 = terraform(0, 0, "plain")
  r1 = terraform(1, 0, "desert")
  r2 = terraform(2, 0, "glacier")
  r3 = terraform(3, 0, "plain")
  r4 = terraform(4, 0, "glacier")

  r0:add_direction(r4, "Wirbel", "Nimm die Abkürzung, Luke")

  r0:set_road(east, 1.0)
  r1:set_road(west, 1.0)
  r1:set_road(east, 1.0)
  r2:set_road(west, 1.0)
  r2:set_road(east, 1.0)
  r3:set_road(west, 1.0)
  r3:set_road(east, 1.0)
  r4:set_road(west, 1.0)

  orcs = add_faction("orcs@eressea.de", "orc", "de")
  orcs.age = 20

  aqua = add_faction("aqua@eressea.de", "aquarian", "de")
  aqua.age = 20

  bugs = add_faction("bugz@eressea.de", "insect", "de")
  bugs.age = 20

  orc = mkunit(orcs, r0, 10)
  orc:add_item("horse", orc.number*3)
  orc:set_skill("sk_riding", 10)

  -- schiffe zum abtreiben:
  ships = {}
  for i = 1, 100 do
    ships[i] = add_ship("boat", ocean)
  end

  astra = mkunit(orcs, r0, 1)
  astra:add_order("NACH Wirbel")
  astra:add_order("NUMMER EINHEIT astr")

  foot = mkunit(orcs, r0, 1)
  foot:add_order("ROUTE W W")
  foot:add_order("NUMMER EINHEIT foot")

  watch = mkunit(orcs, w2, 1)

  ship = add_ship("boat", ocean)
  cptn = mkunit(aqua, ocean, 1)
  cptn.ship = ship
  cptn:add_order("NACH O")
  cptn:add_order("NUMMER EINHEIT cptn")
  cptn:add_order("BENENNE EINHEIT Landungsleiter")
  cptn:add_order("BENENNE PARTEI Meermenschen")

  swim = mkunit(aqua, ocean, 1)
  swim.ship = ship
  swim:add_order("NACH O")
  swim:add_order("NUMMER EINHEIT swim")
  swim:add_order("BENENNE EINHEIT Landungstruppe")

  -- ein schiff im landesinneren
  ship = add_ship("boat", r0)
  sail = mkunit(aqua, r0, 1)
  sail.ship = ship

  crew = mkunit(aqua, r0, 1)
  crew.ship = ship

  bug = mkunit(bugs, r0, 1)

  crew:add_order("NACH O")
  crew:add_order("NUMMER EINHEIT crew")
  crew:add_order("BENENNE EINHEIT Aussteiger")
  crew:add_order("NUMMER PARTEI aqua")

  sail:add_order("NACH O")
  sail:add_order("NUMMER EINHEIT saiL")
  sail:add_order("BENENNE EINHEIT Aussteiger")

  orc:add_order("NUMMER PARTEI orcs")
  orc:add_order("NUMMER EINHEIT orc")
  orc:add_order("BENENNE EINHEIT Orks")
  orc:add_order("ROUTE O O O P P O W W W W")
  orc:add_order("GIB 0 ALLES Steine")
  orc:add_order("GIB 0 ALLES Holz")
  orc:add_order("TRANSPORTIEREN " .. itoa36(bug.id))

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

end


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
  
end

function test_write()
  read_game("24")
  read_orders("befehle")
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
end

function test_fail()
  plain = terraform(0, 0, "plain")
  skill = 5

  f = add_faction("enno@eressea.de", "human", "de")
  print(f)
end

function run_scripts()
  scripts = { 
    "xmas2004.lua"
  }
  for index in scripts do
    local script = scriptpath .. "/" .. scripts[index]
    print("- loading " .. script)
    if pcall(dofile, script)==0 then
      print("Could not load " .. script)
    end
  end
end

-- test_movement()
-- test_fail()
-- test_handler()
-- test_parser()
-- test_monsters()
-- test_combat()
-- test_rewards()
-- test_give()
-- test_write()

-- test_sail()
-- write_game("../testg.txt")
-- read_game("../testg.txt")

test_movement()
run_scripts()
process_orders()
write_reports() 

if swim.region==ocean then
  print "ERROR: Meermenschen können nicht anschwimmen"
end
if sail.region~=r0 then
  print "ERROR: Kapitän kann Schiff mit NACH ohne VERLASSE verlassen"
end
if crew.region==r0 then
  print "ERROR: Einheiten kann Schiff nicht mit NACH ohne VERLASSE verlassen"
end
drift = false
for i = 1, 100 do
  if ships[i].region ~= ocean then
    drift = true
    break
  end
end
if not drift then
  print "ERROR: Unbemannte Schiffe treiben nicht ab"
end
if foot.region ~= w1 then
  print "ERROR: Fusseinheit hat ihr NACH nicht korrekt ausgeführt"
end
if astra.region ~= r4 then
  print "ERROR: Astraleinheit konnte Wirbel nicht benutzen"
end
