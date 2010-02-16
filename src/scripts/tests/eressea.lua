require "lunit"

module( "e2", package.seeall, lunit.testcase )

function setup()
    free_game()
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
    local f = faction.create("noreply@eressea.de", "human", "de")

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
    assert_equal(r2.id, u1.region.id, "boat with 5 humans did not move")
    assert_not_equal(r2.id, u2.region.id, "boat with too many people has moved")
    assert_not_equal(r2.id, u4.region.id, "boat with too much cargo has moved")
end
    
