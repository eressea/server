local tcname = 'tests.e2.insects'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

function setup()
    eressea.free_game()
    eressea.settings.set("rules.food.flags", "4")
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("NewbieImmunity", "0")
end

function test_move_to_glacier()
    local r = region.create(0, 0, "plain")
    local r2 = region.create(1, 0, "glacier")
    local f = faction.create("insect")
    local u = unit.create(f, r, 1)

    u:clear_orders()
    u:add_order("NACH OST")
    process_orders()
    assert_equal(r, u.region)
end

function test_sail_to_glacier()
    local r = region.create(0, 0, "ocean")
    local r2 = region.create(1, 0, "glacier")
    local f = faction.create("insect")
    local u1 = unit.create(f, r, 1, 'insect')

    u1.ship = ship.create(r, 'boat')
    u1:set_skill("sailing", 10)
    u1:clear_orders()
    u1:add_order("NACH OST")
    process_orders()
    assert_equal(r, u1.region)
end 

function test_passenger_into_glacier()
    local r = region.create(0, 0, "ocean")
    local r2 = region.create(1, 0, "glacier")
    local f = faction.create("insect")
    local u1 = unit.create(f, r, 1, 'human')
    local u2 = unit.create(f, r, 1, 'insect')

    u1.ship = ship.create(r, 'boat')
    u1:set_skill("sailing", 10)
    u1:clear_orders()
    u1:add_order("NACH OST")
    u2.ship = u1.ship
    process_orders()
    assert_equal(r2, u2.region)
end 

function test_recruit_in_winter()
    local r = region.create(0, 0, "plain")
    local f = faction.create("insect")
    local u = unit.create(f, r, 1)

    u:add_item('money', 1000)
    u:clear_orders()
    u:add_order("REKRUTIERE 1")
    set_turn(1010)
    process_orders()
    assert_equal('winter', get_season(get_turn()))
    assert_equal(1, u.number)

    u:clear_orders()
    u:add_order("REKRUTIERE 1")
    set_turn(1011)
    process_orders()
    assert_equal('spring', get_season(get_turn()))
    assert_equal(2, u.number)
end 

function test_recruit_in_desert()
    local r = region.create(0, 0, "desert")
    local f = faction.create("insect")
    local u = unit.create(f, r, 1)

    u:add_item('money', 1000)
    u:clear_orders()
    u:add_order("REKRUTIERE 1")
    set_turn(1010)
    process_orders()
    assert_equal('winter', get_season(get_turn()))
    assert_equal(2, u.number)
end 

function test_ride_through_mountain()
    local r1 = region.create(1, 0, "plain")
    local r2 = region.create(2, 0, "mountain")
    local r3 = region.create(3, 0, "plain")
    local f = faction.create("insect")
    local u = unit.create(f, r1, 1)
    u.name="Hercules"
    u:add_order('NACH O O')
    u:add_item('horse', 1)
    u:set_skill('riding', 1, true)
    process_orders()
    assert_equal(r3, u.region)
end

function test_ride_from_mountain()
    local r1 = region.create(1, 0, "mountain")
    local r2 = region.create(2, 0, "plain")
    local r3 = region.create(3, 0, "plain")
    local f = faction.create("insect")
    local u = unit.create(f, r1, 1)
    u:add_order('NACH O O')
    u:add_item('horse', 1)
    u:set_skill('riding', 1, true)
    process_orders()
    assert_equal(r2, u.region)
end
