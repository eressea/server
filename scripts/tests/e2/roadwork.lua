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
    local r = region.create(0, 0, "mountain")
    region.create(1, 0, "mountain")
    u = unit.create(f, r, 1)
    u:add_item('stone', 300) -- no shortage of materials
    u:set_skill('roadwork', 1)
    u:add_order('MACHE Strasse OST')
    eressea.settings.set("rules.food.flags", "4")
end

function teardown()
    eressea.settings.set("rules.food.flags", "0")
end

function test_build_road()
    u.number = 10
    process_orders()
    assert_equal(290, u:get_item('stone'))
end

function test_build_road_with_potion()
    u.number = 10
    u:add_item('p3', 1)
    u:add_order('BENUTZE 1 Schaffenstrunk')
    process_orders()
    assert_equal(280, u:get_item('stone'))
end

function test_build_road_with_ring()
    u.number = 10
    u:add_item('roqf', u.number)
    process_orders()
    assert_equal(200, u:get_item('stone'))
end

function test_build_road_with_ring_and_potion()
    u:add_item('roqf', u.number)
    u:add_item('p3', 1)
    u:add_order('BENUTZE 1 Schaffenstrunk')
    process_orders()
    assert_equal(289, u:get_item('stone'))
end
