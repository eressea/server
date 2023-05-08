local tcname = 'tests.e2.manufacture'
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
    u:set_skill('weaponsmithing', 6)
    u:add_order('MACHE Schwert')
end

function test_make_items()
    u:add_item('iron', 4)
    process_orders()
    assert_equal(2, u:get_item('sword'))
    assert_equal(2, u:get_item('iron'))
end

function test_make_with_rings()
    u:add_item('roqf', u.number)
    u:add_item('iron', 40)
    process_orders()
    assert_equal(10 * 2, u:get_item('sword'))
    assert_equal(20, u:get_item('iron'))
end

function test_make_with_potion()
    u.number = 10
    u:add_item('iron', 10 * u.number)
    u:add_item('p3', 1)
    u:add_order('BENUTZE 1 Schaffenstrunk')
    process_orders()
    assert_equal(2 * 2 * u.number, u:get_item('sword'))
    assert_equal(6 * u.number, u:get_item('iron'))
end

function test_make_with_potion_and_ring()
    u.number = 10
    u:add_item('roqf', u.number)
    u:add_item('iron', 100 * u.number)
    u:add_item('p3', 1)
    u:add_order('BENUTZE 1 Schaffenstrunk')
    process_orders()
    assert_equal(22 * u.number, u:get_item('sword')) -- kaputt!
    assert_equal(78 * u.number, u:get_item('iron'))
end
