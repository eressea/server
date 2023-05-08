local tcname = 'tests.e2.produce'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

local u, r

function setup()
    eressea.free_game()
    local f = faction.create("human")
    r = region.create(0, 0, "plain")
    r:set_resource('tree', 500)
    u = unit.create(f, r, 1)
    u:set_skill('forestry', 2)
    u:add_order('MACHE Holz')
    eressea.settings.set("rules.grow.formula", "0")
end

function test_produce_logs()
    process_orders()
    assert_equal(2, u:get_item('log'))
    assert_equal(498, r:get_resource('tree'))
end

function test_produce_with_rings()
    u:add_item('roqf', 1)
    process_orders()
    assert_equal(20, u:get_item('log'))
    assert_equal(480, r:get_resource('tree'))
end

function test_produce_with_potion()
    u.number = 10
    u:add_item('p3', 1)
    u:add_order('BENUTZE 1 Schaffenstrunk')
    process_orders()
    assert_equal(40, u:get_item('log'))
    assert_equal(460, r:get_resource('tree'))
end

function test_produce_with_potion_and_ring()
    u.number = 10
    u:add_item('p3', 1)
    u:add_order('BENUTZE 1 Schaffenstrunk')
    u:add_item('roqf', 10)
    process_orders()
    assert_equal(220, u:get_item('log'))
    assert_equal(280, r:get_resource('tree'))
end

function test_produce_with_building()
    u:set_skill('forestry', 3)
    u.building = building.create(u.region, 'sawmill', 10)
    u:add_item('money', 250)
    process_orders()
    assert_equal(4, u:get_item('log'))
    assert_equal(498, r:get_resource('tree'))
end

function test_produce_with_building_and_potion()
    u:set_skill('forestry', 3)
    u.building = building.create(u.region, 'sawmill', 10)
    u:add_item('money', 250)
    u:add_item('p3', 1)
    u:add_order('BENUTZE 1 Schaffenstrunk')
    process_orders()
    assert_equal(8, u:get_item('log'))
    assert_equal(496, r:get_resource('tree'))
end

function test_produce_with_building_and_ring()
    u:set_skill('forestry', 3)
    u.building = building.create(u.region, 'sawmill', 10)
    u:add_item('money', 250)
    u:add_item('roqf', 1)
    process_orders()
    assert_equal(40, u:get_item('log'))
    assert_equal(480, r:get_resource('tree'))
end

function test_produce_with_building_and_ring_and_potion()
    u:set_skill('forestry', 3)
    u.building = building.create(u.region, 'sawmill', 10)
    u:add_item('money', 250)
    u:add_item('roqf', 1)
    u:add_item('p3', 1)
    u:add_order('BENUTZE 1 Schaffenstrunk')
    process_orders()
    assert_equal(44, u:get_item('log'))
    assert_equal(478, r:get_resource('tree'))
end
