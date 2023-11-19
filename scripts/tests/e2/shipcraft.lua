local tcname = 'tests.e2.shipcraft'
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
    u:add_item('log', 300) -- no shortage of materials
    u:set_skill('shipcraft', 4) -- humans get + 1
    u:add_order('MACHE Karavelle')
end

function test_build_ship()
    u.number = 10
    process_orders()
    assert_not_nil(u.ship)
    assert_equal(16, u.ship.size)
    assert_equal('caravel', u.ship.type)
    assert_equal(284, u:get_item('log'))
end

function test_build_ship_with_potion()
    u.number = 10
    u:add_item('p3', 1)
    u:add_order('BENUTZE 1 Schaffenstrunk')
    process_orders()
    assert_not_nil(u.ship)
    assert_equal(33, u.ship.size)
    assert_equal(267, u:get_item('log'))
end

function test_build_ship_with_ring()
    u.number = 10
    u:add_item('roqf', u.number)
    process_orders()
    assert_not_nil(u.ship)
    assert_equal(166, u.ship.size)
    assert_equal(134, u:get_item('log'))
end

function test_build_ship_with_ring_and_potion()
    u.number = 10
    u:set_skill('ship', 3)
    u:add_item('roqf', u.number)
    u:add_item('p3', 1)
    u:add_order('BENUTZE 1 Schaffenstrunk')
    process_orders()
    assert_not_nil(u.ship)
    assert_equal(183, u.ship.size)
    assert_equal(117, u:get_item('log'))
end

function test_build_ship_on_ship()
    local sh = ship.create(u.region, 'caravel')
    sh.size = 16
    u.ship = sh
    u.number = 10
    u:set_orders('MACHE SCHIFF')
    process_orders()
    assert_equal(sh, u.ship)
    assert_equal(32, u.ship.size)
    assert_equal(284, u:get_item('log'))
end

function test_build_ship_on_ship_with_type()
    local sh = ship.create(u.region, 'caravel')
    sh.size = 16
    u.ship = sh
    u.number = 10
    u:set_orders('MACHE Karavelle')
    process_orders()
    assert_not_equal(sh, u.ship)
    assert_equal(16, u.ship.size)
    assert_equal(16, sh.size)
    assert_equal(284, u:get_item('log'))
end

function test_build_ship_with_id()
    local sh = ship.create(u.region, 'caravel')
    sh.size = 16
    u.ship = ship.create(u.region, 'longboat')
    u.number = 10
    u:set_orders('MACHE SCHIFF ' .. itoa36(sh.id))
    process_orders()
    assert_not_equal(sh, u.ship)
    assert_equal(32, sh.size)
    assert_equal(284, u:get_item('log'))
end

function test_build_ship_with_type_ignores_id()
    local sh = ship.create(u.region, 'caravel')
    sh.size = 16
    u.number = 10
    u:set_orders('MACHE Karavelle ' .. itoa36(sh.id))
    process_orders()
    assert_not_equal(sh, u.ship)
    assert_equal(16, sh.size)
    assert_equal(16, u.ship.size)
    assert_equal(284, u:get_item('log'))
end

function test_build_ship_with_type_ignores_my_id()
    local sh = ship.create(u.region, 'caravel')
    sh.size = 16
    u.ship = ship.create(u.region, 'longboat')
    u.number = 10
    u:set_orders('MACHE Karavelle ' .. itoa36(sh.id))
    process_orders()
    assert_not_equal(sh, u.ship)
    assert_equal(16, sh.size)
    assert_equal(16, u.ship.size)
    assert_equal('caravel', u.ship.type)
    assert_equal(284, u:get_item('log'))
end
