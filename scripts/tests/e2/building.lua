local tcname = 'tests.e2.building'
local lunit = require('lunit')
if _VERSION >= 'Lua 5.2' then
  _ENV = module(tcname, 'seeall')
else
  module(tcname, lunit.testcase, package.seeall)
end

local u

function setup()
    eressea.free_game()
    local f = faction.create("human")
    local r = region.create(0, 0, "plain")
    u = unit.create(f, r, 1)
    u:set_skill('building', 2)
    u:add_order('MACHE Burg')
    eressea.settings.set("rules.food.flags", "4")
end

function teardown()
    eressea.settings.set("rules.food.flags", "0")
end

function test_build_castle()
    u.number = 10
    u:add_item('stone', 20)
    process_orders()
    assert_not_nil(u.building)
    assert_equal(15, u.building.size)
    assert_equal(5, u:get_item('stone'))
end

function test_build_castle_with_potion()
    u.number = 10
    u:add_item('stone', 30)
    u:add_item('p3', 1)
    u:add_order('BENUTZE 1 Schaffenstrunk')
    process_orders()
    assert_not_nil(u.building)
    assert_equal(25, u.building.size)
    assert_equal(5, u:get_item('stone'))
end

function test_build_castle_with_ring()
    u.number = 10
    u:set_skill('building', 3)
    u:add_item('stone', 300)
    u:add_item('roqf', u.number)
    process_orders()
    assert_not_nil(u.building)
    assert_equal(120, u.building.size)
    assert_equal(180, u:get_item('stone'))
end

function test_build_castle_with_ring_and_potion()
    u.number = 10
    u:set_skill('building', 3)
    u:add_item('stone', 300)
    u:add_item('roqf', u.number)
    u:add_item('p3', 1)
    u:add_order('BENUTZE 1 Schaffenstrunk')
    process_orders()
    assert_not_nil(u.building)
    assert_equal(130, u.building.size)
    assert_equal(170, u:get_item('stone'))
end
