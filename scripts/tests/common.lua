require "lunit"

local function _test_create_ship(r)
    local s = ship.create(r, config.ships[1])
    return s
end

local function one_unit(r, f)
  local u = unit.create(f, r, 1)
  u:add_item("money", u.number * 100)
  u:clear_orders()
  return u
end

local function two_units(r, f1, f2)
  return one_unit(r, f1), one_unit(r, f2)
end

local function two_factions()
  local f1 = faction.create("one@eressea.de", "human", "de")
  local f2 = faction.create("two@eressea.de", "elf", "de")
  return f1, f2
end

module("tests.common", package.seeall, lunit.testcase)

function setup()
    eressea.free_game()
    eressea.settings.set("nmr.removenewbie", "0")
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("NewbieImmunity", "0")
    eressea.settings.set("rules.economy.food", "4")
    eressea.settings.set("rules.encounters", "0")
    eressea.settings.set("rules.peasants.growth", "1")
    eressea.settings.set("study.random_progress", "0")
end

function test_flags()
    local r = region.create(0, 0, "plain")
    local f = faction.create("flags@eressea.de", "halfling", "de")
    local u = unit.create(f, r, 1)
    local no = itoa36(f.id)
    local flags = 50332673
    f.flags = flags

    eressea.write_game("test.dat")
    eressea.free_game()
    eressea.read_game("test.dat")
    os.remove('test.dat')
    f = get_faction(no)
    assert_equal(flags, f.flags)
end

function test_elvenhorse_requires_riding_5()
    local r = region.create(0, 0, "plain")
    region.create(1, 0, "plain")
    local goal = region.create(2, 0, "plain")
    local f = faction.create("riding@eressea.de", "halfling", "de")
    local u = unit.create(f, r, 1)
    u:add_item("elvenhorse", 1)
    u:set_skill("riding", 6)-- halfling has -1 modifier
    u:clear_orders()
    u:add_order("NACH O O")
    process_orders()
    assert_equal(goal, u.region)
end

function test_cannot_ride_elvenhorse_without_enough_skill()
    local r = region.create(0, 0, "plain")
    local goal = region.create(1, 0, "plain")
    region.create(2, 0, "plain")
    local f = faction.create("elvenhorse@eressea.de", "halfling", "de")
    local u = unit.create(f, r, 1)
    u:add_item("elvenhorse", 1)
    u:set_skill("riding", 5) -- halfling has -1 modifier
    u:clear_orders()
    u:add_order("NACH O O")
    process_orders()
    assert_equal(goal, u.region)
end

function test_no_peasant_growth()
    local r = region.create(0, 0, "plain")
    r:set_resource("peasant", 2000)
    eressea.settings.set("rules.peasants.growth", "0")
    process_orders()
    assert_equal(r:get_resource("peasant"), 2000)
end

function test_demon_food()
    local r = region.create(0, 0, "plain")
    local f = faction.create("demonfood@eressea.de", "demon", "de")
    local u = unit.create(f, r, 1)
    local p = r:get_resource("peasant")
    r:set_resource("peasant", 2000)
    eressea.settings.set("rules.economy.food", "0")
    eressea.settings.set("rules.peasants.growth", "0")
    process_orders()
    assert_not_nil(u)
    assert_equal(1, u.number)
    assert_equal(1999, r:get_resource("peasant"))
end

function test_fleeing_units_can_be_transported()
  local r = region.create(0, 0, "plain")
  local r1 = region.create(1, 0, "plain")
  local f1, f2 = two_factions()
  local u1, u2 = two_units(r, f1, f2)
  local u3 = one_unit(r, f2)
  u1.number = 100
  u1:add_order("ATTACKIEREN " .. itoa36(u2.id))
  u2.number = 100
  u2:add_order("FAHREN " .. itoa36(u3.id))
  u2:add_order("KAEMPFE FLIEHE")
  u3.number = 100
  u3:add_order("KAEMPFE FLIEHE")
  u3:add_order("TRANSPORT " .. itoa36(u2.id))
  u3:add_order("NACH O ")
  u3:set_skill("riding", 2)
  u3:add_item("horse", u2.number)
  u3:add_order("KAEMPFE FLIEHE")
  process_orders()
  assert_equal(u3.region.id, r1.id, "transporter did not move")
  assert_equal(u2.region.id, r1.id, "transported unit did not move")
end

function test_plane()
  local pl = plane.create(0, -3, -3, 7, 7)
  local nx, ny = plane.normalize(pl, 4, 4)
  assert_equal(nx, -3, "normalization failed")
  assert_equal(ny, -3, "normalization failed")
  local f = faction.create("plan@eressea.de", "human", "de")
  f.id = atoi36("tpla")
  local r, x, y
  for x = -3, 3 do for y = -3, 3 do
    r = region.create(x, y, "plain")
    if x==y then
      local u = unit.create(f, r, 1)
    end
  end end
end

function test_pure()
  local r = region.create(0, 0, "plain")
  assert_not_equal(nil, r)
  assert_equal(r, get_region(0, 0))
end

function test_read_write()
  local r = region.create(0, 0, "plain")
  local f = faction.create("readwrite@eressea.de", "human", "de")
  local u = unit.create(f, r)
  u.number = 2
  local fno = f.id
  local uno = u.id
  local result = 0
  assert_equal(r.terrain, "plain")
  result = eressea.write_game("test.dat")
  assert_equal(result, 0)
  assert_not_equal(get_region(0, 0), nil)
  assert_not_equal(get_faction(fno), nil)
  assert_not_equal(get_unit(uno), nil)
  r = nil
  f = nil
  u = nil
  eressea.free_game()
  assert_equal(get_region(0, 0), nil)
  assert_equal(nil, get_faction(fno))
  assert_equal(nil, get_unit(uno))
  result = eressea.read_game("test.dat")
  assert_equal(0, result)
  assert_not_equal(nil, get_region(0, 0))
  assert_not_equal(nil, get_faction(fno))
  assert_not_equal(nil, get_unit(uno))
end

function test_descriptions()
    local info = "Descriptions can be very long. Bug 1984 behauptet, dass es Probleme gibt mit Beschreibungen die laenger als 120 Zeichen sind. This description is longer than 120 characters."
    local r = region.create(0, 0, "plain")
    local f = faction.create("descriptions@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    local s = _test_create_ship(r)
    local b = building.create(r, "castle")
    local uno = u.id
    local fno = f.id
    local sno = s.id
    local bno = b.id
    u.info = info
    r.info = info
    f.info = info
    s.info = info
    b.info = info

    filename = "test.dat"
    eressea.write_game(filename)
    eressea.free_game()
    eressea.read_game(filename)
    assert_equal(info, get_ship(sno).info)
    assert_equal(info, get_unit(uno).info)
    assert_equal(info, get_faction(fno).info)
    assert_equal(info, get_building(bno).info)
    assert_equal(info, get_region(0, 0).info)
end

function test_gmtool()
    local r1 = region.create(1, 0, "plain")
    local r2 = region.create(1, 1, "plain")
    local r3 = region.create(1, 2, "plain")
    gmtool.open()
    gmtool.select(r1, true)
    gmtool.select_at(0, 1, true)
    gmtool.select(r2, true)
    gmtool.select_at(0, 2, true)
    gmtool.select(r3, false)
    gmtool.select(r3, true)
    gmtool.select_at(0, 3, false)
    gmtool.select(r3, false)
    
    local selections = 0
    for r in gmtool.get_selection() do
        selections=selections+1
    end
    assert_equal(2, selections)
    assert_equal(nil, gmtool.get_cursor())

    gmtool.close()
end

function test_faction()
    local r = region.create(0, 0, "plain")
    local f = faction.create("testfaction@eressea.de", "human", "de")
    assert(f)
    f.info = "Spazz"
    assert(f.info=="Spazz")
    f:add_item("donotwant", 42)
    f:add_item("stone", 42)
    f:add_item("sword", 42)
    local items = 0
    for u in f.items do
        items = items + 1
    end
    assert(items==2)
    unit.create(f, r)
    unit.create(f, r)
    local units = 0
    for u in f.units do
        units = units + 1
    end
    assert(units==2)
end

function test_unit()
    local r = region.create(0, 0, "plain")
    local f = faction.create("testunit@eressea.de", "human", "de")
    local u = unit.create(f, r)
    u.number = 20
    u.name = "Enno"
    assert(u.name=="Enno")
    u.info = "Spazz"
    assert(u.info=="Spazz")
    u:add_item("sword", 4)
    assert(u:get_item("sword")==4)
    assert(u:get_pooled("sword")==4)
    u:use_pooled("sword", 2)
    assert(u:get_item("sword")==2)
end

function test_region()
  local r = region.create(0, 0, "plain")
  r:set_resource("horse", 42)
  r:set_resource("money", 45)
  r:set_resource("peasant", 200)
  assert(r:get_resource("horse") == 42)
  assert(r:get_resource("money") == 45)
  assert(r:get_resource("peasant") == 200)
  r.name = nil
  r.info = nil
  assert(r.name=="")
  assert(r.info=="")
  r.name = "Alabasterheim"
  r.info = "Hier wohnen die siebzehn Zwerge"
  assert(tostring(r) == "Alabasterheim (0,0)")
end

function test_building()
  local u
  local f = faction.create("testbuilding@eressea.de", "human", "de")
  local r = region.create(0, 0, "plain")
  local b = building.create(r, "castle")
  u = unit.create(f, r)
  u.number = 1
  u.building = b
  u = unit.create(f, r)
  u.number = 2
  -- u.building = b
  u = unit.create(f, r)
  u.number = 3
  u.building = b
  local units = 0
  for u in b.units do
      units = units + 1
  end
  assert(units==2)
  local r2 = region.create(0, 1, "plain")
  assert(b.region==r)
  b.region = r2
  assert(b.region==r2)
  assert(r2.buildings()==b)
end

function test_message()
  local r = region.create(0, 0, "plain")
  local f = faction.create("testmessage@eressea.de", "human", "de")
  local u = unit.create(f, r)
  local msg = message.create("item_create_spell")
  msg:set_unit("mage", u)
  msg:set_int("number", 1)
  msg:set_resource("item", "sword")
  msg:send_region(r)
  msg:send_faction(f)
  
  return msg
end

function test_hashtable()
  local f = faction.create("noreply1@eressea.de", "human", "de")
  f.objects:set("enno", "smart guy")
  f.objects:set("age", 10)
  assert(f.objects:get("jesus") == nil)
  assert(f.objects:get("enno") == "smart guy")
  assert(f.objects:get("age") == 10)
  f.objects:set("age", nil)
  assert(f.objects:get("age") == nil)
end

function test_events()
  local fail = 1
  local function msg_handler(u, evt)
    str = evt:get(0)
    u2 = evt:get(1)
    assert(u2~=nil)
    assert(str=="Du Elf stinken")
    message_unit(u, u2, "thanks unit, i got your message: " .. str)
    message_faction(u, u2.faction, "thanks faction, i got your message: " .. str)
    message_region(u, "thanks region, i got your message: " .. str)
    fail = 0
  end

  plain = region.create(0, 0, "plain")
  skill = 8

  f = faction.create("noreply2@eressea.de", "elf", "de")
  f.age = 20

  u = unit.create(f, plain)
  u.number = 1
  u:add_item("money", u.number*100)
  u:clear_orders()
  u:add_order("NUMMER PARTEI test")
  u:add_handler("message", msg_handler)
  msg = "BOTSCHAFT EINHEIT " .. itoa36(u.id) .. " Du~Elf~stinken"
  f = faction.create("noreply3@eressea.de", "elf", "de")
  f.age = 20

  u = unit.create(f, plain)
  u.number = 1
  u:add_item("money", u.number*100)
  u:clear_orders()
  u:add_order("NUMMER PARTEI eviL")
  u:add_order(msg)
  process_orders()
  assert(fail==0)
end

function test_recruit2()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply4@eressea.de", "human", "de")
    local u = unit.create(f, r)
    u.number = 1
    u:add_item("money", 2000)
    u:clear_orders()
    u:add_order("MACHE TEMP 1")
    u:add_order("REKRUTIERE 1 Elf")
    u:add_order("REKRUTIERE 1 Mensch")
    u:add_order("REKRUTIERE 1")
    process_orders()
end

function test_guard()
  region.create(1, 0, "plain")
  local r = region.create(0, 0, "plain")
  local f1 = faction.create("noreply5@eressea.de", "human", "de")
  f1.age = 20
  local u1 = unit.create(f1, r, 10)
  u1:add_item("sword", 10)
  u1:add_item("money", 10)
  u1:set_skill("melee", 10)
  u1:clear_orders()
  u1:add_order("NACH O")
  u1.name="Kalle Pimp"

  local f2 = faction.create("noreply6@eressea.de", "human", "de")
  f2.age = 20
  local u2 = unit.create(f2, r, 1)
  local u3 = unit.create(f2, r, 1)
  local b = building.create(r, "castle")
  b.size = 10
  u2.building = b
  u3.building = b
  u2:clear_orders()
  u2:add_order("ATTACKIEREN " .. itoa36(u1.id)) -- you will die...
  u2:add_item("money", 100)
  u3:add_item("money", 100)
  process_orders()
  assert_equal(r, u1.region, "unit may not move after combat")
end

function test_recruit()
  local r = region.create(0, 0, "plain")
  local f = faction.create("noreply7@eressea.de", "human", "de")
  local u = unit.create(f, r)
  u.number = 1
  local n = 3
  r:set_resource("peasant", 200)
  u:clear_orders()
  u:add_item("money", 110*n+20)
  u:add_order("REKRUTIERE " .. n)
  process_orders()
  assert(u.number == n+1)
  local p = r:get_resource("peasant")
  assert(p<200 and p>=200-n)
  -- assert(u:get_item("money")==10)
end

function test_produce()
  local r = region.create(0, 0, "plain")
  local f = faction.create("noreply8@eressea.de", "human", "de")
  local u = unit.create(f, r, 1)
  u:clear_orders()
  local sword = config.get_resource('sword')
  u:set_skill(sword.build_skill_name, 3)
  u:add_item("iron", 10)
  u:add_item("money", u.number * 10)
  u:add_order("MACHE Schwert")
  process_orders()
  assert_equal(10-3/sword.build_skill_min*sword.materials['iron'], u:get_item("iron"))
  assert_equal(3/sword.build_skill_min, u:get_item("sword"))
end

function test_work()
  local r = region.create(0, 0, "plain")
  local f = faction.create("noreply9@eressea.de", "human", "de")
  local u = unit.create(f, r, 1)
  u:add_item("money", u.number * 10) -- humans cost 10
  u:set_skill("herbalism", 5)
  u:clear_orders()
  u:add_order("ARBEITEN")
  process_orders()
  assert(u:get_item("money")>=10)
end

function test_upkeep()
    eressea.settings.set("rules.economy.food", "0")
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply10@eressea.de", "human", "de")
    local u = unit.create(f, r, 5)
    u:add_item("money", u.number * 11)
    u:clear_orders()
    u:add_order("LERNE Waffenbau")
    process_orders()
    assert(u:get_item("money")==u.number)
end

function test_id()
  local r = region.create(0, 0, "plain")

  local f = faction.create("noreply11@eressea.de", "human", "de")
  f.id = atoi36("42")
  assert(get_faction(42)~=f)
  assert(get_faction("42")==f)
  assert(get_faction(atoi36("42"))==f)

  local u = unit.create(f, r, 1)
  u.id = atoi36("42")
  assert(get_unit(42)~=u)
  assert(get_unit("42")==u)
  assert(get_unit(atoi36("42"))==u)

  local b = building.create(r, "castle")
  -- <not working> b.id = atoi36("42")
  local fortytwo = itoa36(b.id)
  assert(get_building(fortytwo)==b)
  assert(get_building(atoi36(fortytwo))==b)

  local s = _test_create_ship(r)
  assert_not_nil(s)
  -- <not working> s.id = atoi36("42")
  local fortytwo = itoa36(s.id)
  assert(get_ship(fortytwo)==s)
  assert(get_ship(atoi36(fortytwo))==s)
end

function test_herbalism()
  local r = region.create(0, 0, "plain")
  local f = faction.create("noreply12@eressea.de", "human", "de")
  local u = unit.create(f, r, 1)
  u:add_item("money", u.number * 100)
  u:set_skill("herbalism", 5)
  u:clear_orders()
  u:add_order("MACHE Samen")
  process_orders()
end

function test_mallorn()
    local r = region.create(0, 0, "plain")
    r:set_flag(1, false) -- not mallorn
    r:set_resource("tree", 100)
    assert(r:get_resource("tree")==100)
    local m = region.create(0, 0, "plain")
    m:set_flag(1, true) -- mallorn
    m:set_resource("tree", 100)
    assert(m:get_resource("tree")==100)

    local f = faction.create("noreply13@eressea.de", "human", "de")

    local u1 = unit.create(f, r, 1)
    u1:add_item("money", u1.number * 100)
    u1:set_skill("forestry", 2)
    u1:clear_orders()
    u1:add_order("MACHE HOLZ")

    local u2 = unit.create(f, m, 1)
    u2:add_item("money", u2.number * 100)
    u2:set_skill("forestry", 2)
    u2:clear_orders()
    u2:add_order("MACHE HOLZ")

    local u3 = unit.create(f, m, 1)
    u3:add_item("money", u3.number * 100)
    u3:set_skill("forestry", 2)
    u3:clear_orders()
    u3:add_order("MACHE Mallorn")

    process_orders()

    assert_equal(2, u1:get_item("log"))
    assert_equal(2, u2:get_item("log"))
    local mallorn_cfg = config.get_resource("mallorn")
    if mallorn_cfg then
        assert_equal(1, u3:get_item("mallorn"))
    else
        assert_equal(-1, u3:get_item("mallorn"))
        assert_equal(0, u3:get_item("log"))
    end
end

function test_coordinate_translation()
    local pl = plane.create(1, 500, 500, 1001, 1001) -- astralraum
    local pe = plane.create(1, -8761, 3620, 23, 23) -- eternath
    local r = region.create(1000, 1000, "plain")
    local f = faction.create("noreply14@eressea.de", "human", "de")
    assert_not_equal(nil, r)
    assert_equal(r.x, 1000)
    assert_equal(r.y, 1000)
    local nx, ny = plane.normalize(pl, r.x, r.y)
    assert_equal(nx, 1000)
    assert_equal(ny, 1000)
    local r1 = region.create(500, 500, "plain")
    f:set_origin(r1)
    nx, ny = f:normalize(r1)
    assert_equal(0, nx)
    assert_equal(0, ny)
    local r0 = region.create(0, 0, "plain")
    nx, ny = f:normalize(r0)
    assert_equal(0, nx)
    assert_equal(0, ny)
    nx, ny = f:normalize(r)
    assert_equal(500, nx)
    assert_equal(500, ny)
    local rn = region.create(1010, 1010, "plain")
    nx, ny = f:normalize(rn)
    assert_equal(-491, nx)
    assert_equal(-491, ny)

    local re = region.create(-8760, 3541, "plain") -- eternath
    nx, ny = f:normalize(rn)
    assert_equal(-491, nx)
    assert_equal(-491, ny)
end

function test_control()
    local u1, u2 = two_units(region.create(0, 0, "plain"), two_factions())
    local r = u1.region
    local b = building.create(r, "castle")
    u1.building = b
    u2.building = b
    assert_equal(u1, b.owner)
    u1:clear_orders()
    u1:add_order("GIB " .. itoa36(u2.id) .. " KOMMANDO")
    u1:add_order("VERLASSE")
    process_orders()
    assert_equal(u2, b.owner)
end

function test_building_other()
    local r = region.create(0,0, "plain")
    local f1 = faction.create("noreply17@eressea.de", "human", "de")
    local f2 = faction.create("noreply18@eressea.de", "human", "de")
    local b = building.create(r, "castle")
    b.size = 10
    local u1 = unit.create(f1, r, 3)
    u1.building = b
    u1:add_item("money", 100)

    local u2 = unit.create(f2, r, 3)
    u2:set_skill("building", 10)
    u2:add_item("money", 100)
    u2:add_item("stone", 100)
    u2:clear_orders()
    u2:add_order("MACHEN BURG " .. itoa36(b.id))
    process_orders()
    assert_not_equal(10, b.size)
end

-- segfault above

function test_config()
  assert_not_equal(nil, config.basepath)
  assert_not_equal(nil, config.locales)
end

local function _test_create_laen()
    eressea.settings.set("rules.terraform.all", "1")
    local r = region.create(0,0, "mountain")
    local f1 = faction.create("noreply19@eressea.de", "human", "de")
    local u1 = unit.create(f1, r, 1)
    r:set_resource("laen", 50)
    return r, u1
end

function test_laen1()
  local r, u1 = _test_create_laen()
  
  u1:add_item("money", 1000)
  u1:set_skill("mining", 14)
  u1:clear_orders()
  u1:add_order("MACHEN Laen")
 
  process_orders()
  assert_equal(0, u1:get_item("laen"))
end

function test_laen2()
  local r, u1 = _test_create_laen()
  
  u1:add_item("money", 1000)
  u1:set_skill("mining", 15)
  u1:clear_orders()
  u1:add_order("MACHEN Laen")
  u1.name = "Laenmeister"

  local b = building.create(r, "mine")
  b.size = 10
  u1.building = b
  local laen = r:get_resource("laen")
 
  process_orders()
  init_reports()
  write_report(u1.faction)
  assert_equal(laen - 2, r:get_resource("laen"))
  assert_equal(2, u1:get_item("laen"))
end

function test_mine()
  local r = region.create(0,0, "mountain")
  local f1 = faction.create("noreply20@eressea.de", "human", "de")
  local u1 = unit.create(f1, r, 1)
  
  u1:add_item("money", 1000)
  u1:set_skill("mining", 1)
  u1:clear_orders()
  u1:add_order("MACHEN Eisen")
 
  local b = building.create(r, "mine")
  b.size = 10
  u1.building = b
  local iron = r:get_resource("iron")
 
  process_orders()
  assert_equal(2, u1:get_item("iron")) -- skill +1
  assert_equal(iron - 1, r:get_resource("iron")) -- only 1/2 is taken away
end

function test_guard_resources()
  -- this is not quite http://bugs.eressea.de/view.php?id=1756
  local r = region.create(0,0, "mountain")
  local f1 = faction.create("noreply21@eressea.de", "human", "de")
  f1.age=20
  local f2 = faction.create("noreply22@eressea.de", "human", "de")
  f2.age=20
  local u1 = unit.create(f1, r, 1)
  u1:add_item("money", 100)
  u1:set_skill("melee", 3)
  u1:add_item("sword", 1)
  u1:clear_orders()
  u1:add_order("BEWACHEN")
  
  local u2 = unit.create(f2, r, 1)
  u2:add_item("money", 100)
  u2:set_skill("mining", 3)
  u2:clear_orders()
  u2:add_order("MACHEN EISEN")
 
  process_orders()
  local iron = u2:get_item("iron")
  assert_true(iron > 0)
  process_orders()
  assert_equal(iron, u2:get_item("iron"))
end

local function is_flag_set(flags, flag)
  return math.fmod(flags, flag*2) - math.fmod(flags, flag) == flag;
end

function test_hero_hero_transfer()
  local r = region.create(0,0, "mountain")
  local f = faction.create("noreply23@eressea.de", "human", "de")
  f.age=20
  local UFL_HERO = 128
  
  local u1 = unit.create(f, r, 1)
  local u2 = unit.create(f, r, 1)
  u1:add_item("money", 10000)
  u1.flags = u1.flags + UFL_HERO
  u2.flags = u2.flags + UFL_HERO
  u1:clear_orders()
  u1:add_order("GIB " .. itoa36(u2.id) .. " 1 PERSONEN")
  u1:add_order("REKRUTIEREN 1")
  process_orders()
  assert_equal(2, u2.number)
  assert_true(is_flag_set(u2.flags, 128), 128, "unit is not a hero?")
  assert_equal(1, u1.number)
  assert_false(is_flag_set(u1.flags, 128), 128, "recruiting into an empty hero unit should not create a hero")
end

function test_hero_normal_transfer()
  local r = region.create(0,0, "mountain")
  local f = faction.create("noreply24@eressea.de", "human", "de")
  f.age=20
  local UFL_HERO = 128
  
  local u1 = unit.create(f, r, 1)
  local u2 = unit.create(f, r, 1)
  u1:add_item("money", 10000)
  u1.flags = u1.flags + UFL_HERO
  u1:clear_orders()
  u1:add_order("GIB " .. itoa36(u2.id) .. " 1 PERSONEN")
  process_orders()
  assert_equal(1, u1.number)
  assert_equal(1, u2.number)
  assert_true(is_flag_set(u1.flags, 128), "unit is not a hero?")
  assert_false(is_flag_set(u2.flags, 128), "unit turned into a hero")
end

function test_expensive_skills_cost_money()
  local r = region.create(0,0, "mountain")
  local f = faction.create("noreply25@eressea.de", "elf", "de")
  local u = unit.create(f, r, 1)
  u:add_item("money", 10000)
  u:clear_orders()
  u:add_order("LERNEN MAGIE Gwyrrd")
  assert_equal(0, u:get_skill("magic"))
  process_orders()
  assert_equal(9900, u:get_item("money"))
  assert_equal(1, u:get_skill("magic"))
end

function test_food_is_consumed()
  local r = region.create(0, 0, "plain")
  local f = faction.create("noreply26@eressea.de", "human", "de")
  local u = unit.create(f, r, 1)
  u:add_item("money", 100)
  u:clear_orders()
  u:add_order("LERNEN Reiten") -- don't work
  eressea.settings.set("rules.economy.food", "4")
  process_orders()
  assert_equal(100, u:get_item("money"))
end

function test_food_can_override()
  local r = region.create(0, 0, "plain")
  local f = faction.create("noreply27@eressea.de", "human", "de")
  local u = unit.create(f, r, 1)
  u:add_item("money", 100)
  u:clear_orders()
  u:add_order("LERNEN Reiten") -- don't work
  eressea.settings.set("rules.economy.food", "0")
  process_orders()
  assert_equal(90, u:get_item("money"))
end

function test_swim_and_survive()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply28@eressea.de", "human", "de")
    f.nam = "chaos"
    local u = unit.create(f, r, 1)
    process_orders()
    r.terrain = "ocean"
    local s = _test_create_ship(r)
    u:clear_orders()
    u:add_order("BETRETE SCHIFF " .. itoa36(s.id))
    process_orders()
    assert_equal(u.number, 1)
end

function test_swim_and_die()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply29@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    local uid = u.id
    process_orders()
    r.terrain = "ocean"
    u = get_unit(uid)
    assert_not_equal(get_unit(uid), nil)
    process_orders()
    assert_equal(get_unit(uid), nil)
end

function test_ride_with_horse()
    region.create(1, 0, "plain")
    region.create(2, 0, "plain")
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply30@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u:add_item("horse", 1)
    local horse_cfg = config.get_resource("horse")
    u:add_item("sword", (horse_cfg.capacity - u.weight)/100)
    u:set_skill("riding", 2)

    u:clear_orders()
    u:add_order("NACH O O")
    process_orders()
    assert_equal(u.region.x, 2)

    u:add_item("sword", 1)
    u:clear_orders()
    u:add_order("NACH W W")
    process_orders()
    assert_equal(u.region.x, 1)
end

function test_ride_with_horses_and_cart()
    region.create(1, 0, "plain")
    region.create(2, 0, "plain")
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply31@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    local horse_cfg = config.get_resource("horse")
    local cart_cfg = config.get_resource("cart")
    local sword_cfg = config.get_resource("sword")

    u:set_skill("riding", 3)

    local capacity = (horse_cfg.capacity-horse_cfg.weight)*2 - u.weight
    if cart_cfg~=nil then
        capacity = capacity + cart_cfg.capacity-cart_cfg.weight
    end
    u:add_item("sword", capacity / sword_cfg.weight)

    u:add_item("horse", 1)
    if cart_cfg~=nil then
        -- we need 2 horses for a cart, so this should fail:
        u:add_item("cart", 1)
        u:clear_orders()
        u:add_order("NACH O O")
        process_orders()
        assert_equal(0, u.region.x)
    end

    -- here is your second horse, milord:
    u:add_item("horse", 1)
    assert_equal(2, u:get_item("horse"))

    -- ride
    u:clear_orders()
    u:add_order("NACH O O")
    process_orders()
    assert_equal(2, u.region.x)

    -- walk
    u:add_item("sword", 1000/sword_cfg.weight)
    u:clear_orders()
    u:add_order("NACH W W")
    process_orders()
    assert_equal(1, u.region.x)
    
    -- make this fellow too heavy
    u:add_item("sword", 1000/sword_cfg.weight)
    u:clear_orders()
    u:add_order("NACH W W")
    process_orders()
    assert_equal(1, u.region.x)
end

function test_walk_and_carry_the_cart()
    region.create(1, 0, "plain")
    local r = region.create(2, 0, "plain")
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply32@eressea.de", "human", "de")
    local u = unit.create(f, r, 10)
    u:add_item("cart", 1)

    -- walk
    u:clear_orders()
    u:add_order("NACH O O")
    process_orders()
    assert_equal(1, u.region.x)
end

module("tests.recruit", package.seeall, lunit.testcase)

function setup()
    eressea.free_game()
    eressea.settings.set("rules.economy.food", "4")
    eressea.settings.set("rules.peasants.growth", "0")
end

function test_bug_1795_limit()
  local r = region.create(0, 0, "plain")    
  local f = faction.create("noreply@eressea.de", "human", "de")
  local u1 = one_unit(r,f) 
  u1:add_item("money", 100000000)
  u1:add_order("REKRUTIEREN 9999")
  r:set_resource("peasant", 2000) -- no fractional growth!
  local peasants = r:get_resource("peasant")
  local limit,frac = math.modf(peasants/40) -- one day this should be a parameter
  
  process_orders()
  assert_equal(limit+1, u1.number, u1.number .. "!=" .. (limit+1))
  assert_equal(peasants-limit, r:get_resource("peasant"))
end

function test_bug_1795_demons()
  local r = region.create(0, 0, "plain")    
  local f = faction.create("noreply@eressea.de", "demon", "de")
  local u1 = one_unit(r,f) 
  r:set_resource("peasant", 2000)
  local peasants = r:get_resource("peasant")
  local limit,frac = math.modf(peasants/40) 

  u1:add_item("money", 100000000)
  u1:add_order("REKRUTIEREN 9999")
  
  process_orders()
  
  assert_equal(limit+1, u1.number, u1.number .. "!=" .. (limit+1))
  assert_equal(peasants, r:get_resource("peasant"))
end

module("tests.report", package.seeall, lunit.testcase)

function setup()
    eressea.free_game()
    eressea.settings.set("nmr.removenewbie", "0")
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("rules.economy.food", "4")
end

local function find_in_report(f, pattern, extension)
    extension = extension or "nr"
    local filename = config.reportpath .. "/" .. get_turn() .. "-" .. itoa36(f.id) .. "." .. extension
    local report = io.open(filename, 'r');
    assert_not_nil(report)
    t = report:read("*all")
    report:close()

    local start, _ = string.find(t, pattern)
    return start~=nil
end

function test_coordinates_no_plane()
    local r = region.create(0, 0, "mountain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    init_reports()
    write_report(f)
    assert_true(find_in_report(f, r.name .. " %(0,0%), Berg"))
end

function test_show_shadowmaster_attacks()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u.race = "shadowmaster"
    u:clear_orders()
    u:add_order("ZEIGE Schattenmeister")
    process_orders()
    init_reports()
    write_report(f)
    assert_false(find_in_report(f, ", ,"))
end

function test_coordinates_named_plane()
    local p = plane.create(0, -3, -3, 7, 7, "Hell")
    local r = region.create(0, 0, "mountain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    init_reports()
    write_report(f)
    assert_true(find_in_report(f, r.name .. " %(0,0,Hell%), Berg"))
end

function test_coordinates_unnamed_plane()
    local p = plane.create(0, -3, -3, 7, 7)
    local r = region.create(0, 0, "mountain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    init_reports()
    write_report(f)
    assert_true(find_in_report(f, r.name .. " %(0,0%), Berg"))
end

function test_coordinates_noname_plane()
    local p = plane.create(0, -3, -3, 7, 7, "")
    local r = region.create(0, 0, "mountain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    init_reports()
    write_report(f)
    assert_true(find_in_report(f, r.name .. " %(0,0%), Berg"))
end

function test_lighthouse()
    eressea.free_game()
    local r = region.create(0, 0, "mountain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    region.create(1, 0, "mountain")
    region.create(2, 0, "ocean")
    region.create(0, 1, "firewall")
    region.create(3, 0, "mountain")
    region.create(4, 0, "plain")
    local u = unit.create(f, r, 1)
    local b = building.create(r, "lighthouse")
    b.size = 100
    b.working = true
    u.building = b
    u:set_skill("perception", 9)
    u:add_item("money", 1000)
    assert_not_nil(b)

    init_reports()
    write_report(f)
    assert_true(find_in_report(f, " %(1,0%) %(vom Turm erblickt%)"))
    assert_true(find_in_report(f, " %(2,0%) %(vom Turm erblickt%)"))
    assert_true(find_in_report(f, " %(3,0%) %(vom Turm erblickt%)"))

    assert_false(find_in_report(f, " %(0,0%) %(vom Turm erblickt%)"))
    assert_false(find_in_report(f, " %(0,1%) %(vom Turm erblickt%)"))
    assert_false(find_in_report(f, " %(4,0%) %(vom Turm erblickt%)"))
end

module("tests.parser", package.seeall, lunit.testcase)

function setup()
    eressea.free_game()
    eressea.settings.set("rules.economy.food", "4") -- FOOD_IS_FREE
    eressea.settings.set("rules.encounters", "0")
    eressea.settings.set("rules.move.owner_leave", "0")
end

function test_parser()
    local r = region.create(0, 0, "mountain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    local filename = "orders.txt"
    
    local file = io.open(filename, "w")
    assert_not_nil(file)
    file:write('ERESSEA ' .. itoa36(f.id) .. ' "' .. f.password .. '"\n')
    file:write('EINHEIT ' .. itoa36(u.id) .. "\n")
    file:write("BENENNEN EINHEIT 'Goldene Herde'\n")
    file:close()
    
    eressea.read_orders(filename)
    process_orders()
    assert_equal("Goldene Herde", u.name)
end
