require "lunit"

module("tests.e2.e2features", package.seeall, lunit.testcase )

local function one_unit(r, f)
  local u = unit.create(f, r, 1)
  u:add_item("money", u.number * 100)
  u:clear_orders()
  return u
end

local function two_factions()
  local f1 = faction.create("one@eressea.de", "human", "de")
  local f2 = faction.create("two@eressea.de", "human", "de")
  return f1, f2
end

local function two_units(r, f1, f2)
  return one_unit(r, f1), one_unit(r, f2)
end

function setup()
    eressea.free_game()
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("rules.economy.food", "4")
    eressea.settings.set("rules.ship.storms", "0")
end

function test_learn()
    eressea.settings.set("study.random_progress", "0")
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    f.age = 20
    local u = unit.create(f, r)
    u:clear_orders()
    u:add_order("@LERNEN Reiten")
    process_orders()
    assert_equal(1, u:get_skill("riding"))
    process_orders()
    process_orders()
    assert_equal(2, u:get_skill("riding"))
    process_orders()
    process_orders()
    process_orders()
    assert_equal(3, u:get_skill("riding"))
end

function test_teach()
    eressea.settings.set("study.random_progress", "0")
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    f.age = 20
    local u = unit.create(f, r, 10)
    local u2 = unit.create(f, r)
    u:clear_orders()
    u:add_order("@LERNEN reiten")
    u2:clear_orders()
    u2:add_order("LEHREN " .. itoa36(u.id))
    u2:set_skill("riding", 4)
    process_orders()
    assert_equal(1, u:get_skill("riding"))
    process_orders()
    assert_equal(2, u:get_skill("riding"))
end

function test_rename()
  local r = region.create(0, 0, "plain")
  local f = faction.create("noreply@eressea.de", "human", "de")
  local u = unit.create(f, r)
  u:add_item("aoh", 1)
  assert_equal(u:get_item("ao_healing"), 1)
end

function DISABLE_test_alp()
    local r = region.create(0,0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    local u2 = unit.create(f, r, 1)
    u.race = "elf"
    u:set_skill("magic", 10)
    u:add_item("money", 3010)
    u.magic = "illaun"
    u.aura = 200
    u.ship = s1
    u:add_spell("summon_alp")
    u:clear_orders()
    u:add_order("ZAUBERE 'Alp' " .. itoa36(u2.id))
    process_orders()
    print(get_turn(), f)
    write_reports()
end

function test_unit_limit_is_1500()
  local r = region.create(0,0, "plain")
  local f = faction.create("noreply@eressea.de", "human", "de")
  for i = 1,1500 do
    unit.create(f, r, 1)
  end
  local u = unit.create(f, r, 0)
  u:add_item("money", 20000)
  u:clear_orders()
  u:add_order("REKRUTIEREN 1")
  process_orders()
  assert_equal(1, u.number)
end

function test_ship_capacity()
    local r = region.create(0,0, "ocean")
    region.create(1,0, "ocean")
    local r2 = region.create(2,0, "ocean")
    local f = faction.create("capacity@eressea.de", "human", "de")

    -- u1 is at the limit and moves
    local s1 = ship.create(r, "boat")
    local u1 = unit.create(f, r, 5)
    u1.ship = s1
    u1:set_skill("sailing", 10)
    u1:clear_orders()
    u1:add_order("NACH O O")

    -- u2 has too many people
    local s2 = ship.create(r, "boat")
    local u2 = unit.create(f, r, 6)
    u2.ship = s2
    u2:set_skill("sailing", 10)
    u2:clear_orders()
    u2:add_order("NACH O O")

    -- u4 has too much stuff
    local s4 = ship.create(r, "boat")
    local u4 = unit.create(f, r, 5)
    u4.ship = s4
    u4:set_skill("sailing", 10)
    u4:add_item("sword", 1)
    u4:clear_orders()
    u4:add_order("NACH O O")

    process_orders()

--    print(s.region, u.region, r2)
    assert_equal(r2, u1.region, "boat with 5 humans did not move")
    assert_not_equal(r2, u2.region, "boat with too many people has moved")
    assert_not_equal(r2, u4.region, "boat with too much cargo has moved")
end

function test_levitate()
  local r = region.create(0,0, "plain")
  local f = faction.create("noreply@eressea.de", "human", "de")
  local u = unit.create(f, r, 2)
  local s = ship.create(r, "boat")
  u.ship = s
  u.age = 20
  u:set_skill("sailing", 5)
  u:add_item("money", 100)
  u:clear_orders()
  u:add_order("ARBEITE")
  levitate_ship(u.ship, u, 2, 1)
  assert_equal(32, u.ship.flags)
  process_orders()
  assert_equal(0, u.ship.flags)
end

function test_terrains()
  local terrains = { "hell", "wall1", "corridor1" }
  for k,v in ipairs(terrains) do
    local r = region.create(k, k, v)
    assert_not_equal(nil, r)
  end
end

function test_races()
  local races = { "wolf", "orc", "human", "demon" }
  for k,v in ipairs(races) do
    local f = faction.create("noreply@eressea.de", "human", "de")
    assert_not_equal(nil, f)
  end
end

function test_can_give_person()
  local r = region.create(0, 0, "plain")
  local f1 = faction.create("noreply@eressea.de", "human", "de")
  local f2 = faction.create("noreply@eressea.de", "human", "de")
  local u1 = unit.create(f1, r, 10)
  local u2 = unit.create(f2, r, 10)
  u1.faction.age = 10
  u2.faction.age = 10
  u1:add_item("money", 500)
  u2:add_item("money", 500)
  u2:clear_orders()
  u2:add_order("GIB ".. itoa36(u1.id) .. " 1 PERSON")
  u2:add_order("HELFE ".. itoa36(f1.id) .. " GIB")
  u1:add_order("HELFE ".. itoa36(f2.id) .. " GIB")
  u1:add_order("KONTAKTIERE ".. itoa36(u2.id))
  process_orders()
  assert_equal(9, u2.number)
  assert_equal(11, u1.number)
end

function test_no_uruk()
  local f1 = faction.create("noreply@eressea.de", "uruk", "de")
  assert_equal(f1.race, "orc")
end

function test_snowman()
    local r = region.create(0, 0, "glacier")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u:add_item("snowman", 1)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Schneemann")
    process_orders()
    for u2 in r.units do
        if u2.id~=u.id then
            assert_equal(u2.race, "snowman")
            u = nil
            break
        end
    end
    assert_equal(nil, u)
end

function test_block_movement()
  eressea.settings.set("rules.guard.base_stop_prob", "0.3")
  eressea.settings.set("rules.guard.amulet_stop_prob", "0.0")
  eressea.settings.set("rules.guard.skill_stop_prob", "0.1")

  local r0 = region.create(0, 0, "plain")
  local r1 = region.create(1, 0, "plain")
  local r2 = region.create(2, 0, "plain")
  local f1, f2 = two_factions()
  f1.age=20
  f2.age=20

  local u11 = one_unit(r1, f1)
  local u2 = { }
  for i = 1, 20 do
    u2[i] = one_unit(r0, f2)
  end
  
  u11:add_item("sword", 1)
  u11:add_item("money", 1)
  u11:set_skill("melee", 1)
  u11:set_skill("perception", 7)
  u11:clear_orders()
  u11:add_order("BEWACHEN")
  
  process_orders()
  
  for i, u in ipairs(u2) do
    u:add_item("horse", 1)
  	u:set_skill("riding", 1)
  	u:clear_orders()
  	u:add_order("NACH o o")
  end
  
  u2[1]:set_skill("stealth", 8)
  
  process_orders()

  assert_equal(r2, u2[1].region, "nobody should see me")
  for i, u in ipairs(u2) do
    if i > 1 then
      assert_equal(r1, u.region, "perception +7 should always stop me")
    end
  end
end



function test_block_movement_aots()
  eressea.settings.set("rules.guard.base_stop_prob", "0.0")
  eressea.settings.set("rules.guard.skill_stop_prob", "1.0")
  eressea.settings.set("rules.guard.amulet_stop_prob", "1.0")

  local r0 = region.create(0, 0, "plain")
  local r1 = region.create(1, 0, "plain")
  local r2 = region.create(2, 0, "plain")
  local f1, f2 = two_factions()
  f1.age=20
  f2.age=20

  local u11, u12 = two_units(r1, f1, f1)
  local u21, u22 = two_units(r0, f2, f2)
  
  for i, u in ipairs ({ u11, u12 }) do
    u:add_item("sword", 1)
  	u:add_item("money", 1)
  	u:set_skill("melee", 1)
  	u:clear_orders()
  	u:add_order("BEWACHEN")
  end
  
  process_orders()
  
  for i, u in ipairs ({ u21, u22 }) do
    u:add_item("horse", 1)
  	u:set_skill("riding", 1)
  	u:clear_orders()
  	u:add_order("NACH o o")
  end
  
  u12:add_item("aots", 1)
  u22:set_skill("stealth", 1)

  process_orders()

  assert_equal(r1, u21.region, "unit with amulet should stop me")
  assert_equal(r2, u22.region, "nobody should see me")
end

function test_stonegolems()
  local r0 = region.create(0, 0, "plain")
  local f1 = faction.create("noreply@eressea.de", "stonegolem", "de")
  local u1 = unit.create(f1, r0, 1)
  local u2 = unit.create(f1, r0, 2)
  local c1 = building.create(r0, "castle")

  c1.size = 226

  u1:set_skill("building", 1)
  u2:set_skill("building", 1)

-- test that no server crash occur
  u1:clear_orders()
  u1:add_order("Mache Burg")
  process_orders()
  assert_equal(0 ,u1.number, "There shoud be no Stone Golems")
-- end test server crash

-- test that Stone Golems build for four stones
  u2:clear_orders()
  u2:add_order("MACHE 4 BURG " .. itoa36(c1.id))
  process_orders()
  assert_equal(230, c1.size, "resulting size should be 230")
  assert_equal(1 ,u2.number, "There shoud be one Stone Golems")
-- end test Stone Golems four stones
end
